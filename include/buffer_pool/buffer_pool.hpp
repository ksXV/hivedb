#pragma once

#include <spdlog/spdlog.h>

#include <algorithm>
#include <buffer_pool/lru_k.hpp>
#include <cstdint>
#include <disk/disk_scheduler.hpp>
#include <filesystem>
#include <libassert/assert.hpp>
#include <list>
#include <misc/config.hpp>
#include <numeric>
#include <print>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace hivedb {
// TODO: make this thread safe later
struct frame_header {
 public:
  frame_id_t frame_id{INVALID_FRAME_ID};
  bool is_dirty{false};

 private:
  std::int32_t m_pin_count{-99};
  std::vector<char> m_data;

 public:
  explicit frame_header(frame_id_t, std::vector<char> &);

  frame_header() = default;
  frame_header(const frame_header &) = delete;
  frame_header &operator=(const frame_header &) = delete;

  frame_header(frame_header &&) = default;
  frame_header &operator=(frame_header &&) = default;
  ~frame_header() = default;

  void increase_pin_count();
  void decrease_pin_count();

  [[nodiscard]]
  std::int32_t get_pin_count() const;

  [[nodiscard]]
  const char *get_data() const;

  [[nodiscard]]
  char *get_data();
};

template <disk_manager_t T>
struct buffer_pool {
 public:
  const frame_id_t max_frames;

 private:
  static constexpr std::int32_t m_k = 10;

  std::unordered_map<page_id_t, frame_id_t> m_page_table;
  std::vector<frame_header> m_frames;
  std::list<frame_id_t> m_empty_frames;

  disk_scheduler<T> m_scheduler;
  lru_k m_frame_replacer;

  page_id_t m_next_page;

 public:
  buffer_pool() = delete;
  explicit buffer_pool(frame_id_t, const std::filesystem::path &path = "");

  buffer_pool(const buffer_pool<T> &) = delete;
  buffer_pool &operator=(const buffer_pool<T> &) = delete;
  buffer_pool(buffer_pool<T> &&) = delete;
  buffer_pool &operator=(buffer_pool<T> &&) = delete;
  ~buffer_pool() = default;

  [[nodiscard]]
  constexpr page_id_t allocate_new_page();

  [[nodiscard]]
  frame_header &request_page(page_id_t, bool = true);

  frame_id_t evict_page(page_id_t);
  bool flush_page(page_id_t);
  bool flush_pages();
};

template <disk_manager_t T>
buffer_pool<T>::buffer_pool(frame_id_t max_frms,
                            const std::filesystem::path &path)
    : max_frames(max_frms),
      m_empty_frames(max_frames, 0),
      m_scheduler(path),
      m_frame_replacer(m_k, max_frames),
      m_next_page(0) {
  if (max_frames < 0) throw std::invalid_argument("max_frames must be >= 0");

  m_frames.resize(max_frames);
  std::iota(m_empty_frames.begin(), m_empty_frames.end(), 0);
}

template <disk_manager_t T>
constexpr page_id_t buffer_pool<T>::allocate_new_page() {
  return m_next_page++;
}

// Requesting a page WILL INCREASE ITS PIN COUNT!
template <disk_manager_t T>
frame_header &buffer_pool<T>::request_page(page_id_t id, bool should_pin) {
  spdlog::info("Requested page: {}", id);
  if (const auto it = m_page_table.find(id); it != m_page_table.end()) {
    // Found our page yippie
    m_frame_replacer.recordAccess(it->second);
    m_frame_replacer.setEvictable(it->second, false);

    auto &frame = m_frames.at(it->second);
    if (should_pin) frame.increase_pin_count();

    return frame;
  }

  // We hit a page fault :(
  // First get the page from the disk
  std::vector<char> buffer;
  buffer.resize(PAGE_SIZE);
  std::promise<bool> is_done_promise;
  auto is_done = is_done_promise.get_future();

  disk_request req{.type = disk_request_type::read,
                   .data = buffer.data(),
                   .page_id = id,
                   .is_done = std::move(is_done_promise)};
  m_scheduler.schedule(std::move(req));

  is_done.wait();
  if (!is_done.get()) throw std::runtime_error("request_page() failed");

  // Do we have an empty space in the page table?
  if (m_page_table.size() <
      static_cast<decltype(m_page_table)::size_type>(max_frames)) {
    ASSERT(!m_empty_frames.empty());

    const auto frame_id = m_empty_frames.front();
    m_page_table.emplace(id, frame_id);
    m_empty_frames.pop_front();

    ASSERT(m_frames.begin() + frame_id < m_frames.end());
    m_frames[frame_id] = frame_header{frame_id, buffer};
    m_frame_replacer.recordAccess(frame_id);

    auto &frame = m_frames[frame_id];
    if (should_pin) frame.increase_pin_count();
    return frame;
  }

  // We don't :(
  // Maybe scan for a dirty unpinned page and write it
  auto it = std::find_if(m_frames.begin(), m_frames.end(),
                         [](const frame_header &frame) -> bool {
                           return frame.is_dirty && frame.get_pin_count() == 0;
                         });

  if (it != m_frames.end()) {
    frame_id_t frame_id = std::distance(m_frames.begin(), it);

    const auto page_id_to_be_flushed_it =
        std::find_if(m_page_table.begin(), m_page_table.end(),
                     [frame_id](const auto &pair) -> bool {
                       return pair.second == frame_id;
                     });

    ASSERT(page_id_to_be_flushed_it != m_page_table.end());

    const bool status = flush_page(page_id_to_be_flushed_it->first);
    if (!status) throw std::runtime_error("flush_page() failed");

    frame_id = m_empty_frames.front();
    m_page_table.emplace(id, frame_id);
    m_empty_frames.pop_front();

    m_frame_replacer.recordAccess(frame_id);

    ASSERT(m_frames.begin() + frame_id < m_frames.end());
    m_frames[frame_id] = frame_header{frame_id, buffer};
    if (should_pin) m_frames[frame_id].increase_pin_count();

    m_frame_replacer.recordAccess(frame_id);

    return m_frames[frame_id];
  }

  // No dirty unpinned page to flush :(
  // Time to find a victim
  const auto frame_to_evict = m_frame_replacer.evict();

  spdlog::info("Attempting to evict frame {}", frame_to_evict.has_value()
                                                   ? frame_to_evict.value()
                                                   : INVALID_FRAME_ID);

  if (!frame_to_evict.has_value() &&
      frame_to_evict.value() == INVALID_FRAME_ID) {
    throw std::runtime_error(
        "frame_replacer.evict() failed; got -1 or invalid frame");
  }

  if (m_frames.at(frame_to_evict.value()).get_pin_count() > 0) {
    throw std::runtime_error(
        "VICTIM IS PINNED! maybe this a oom? needs more testing.");
  }

  const auto page_id_to_be_removed =
      std::find_if(m_page_table.begin(), m_page_table.end(),
                   [frame_to_evict](const auto &pair) {
                     return pair.second == frame_to_evict.value();
                   });

  ASSERT(page_id_to_be_removed != m_page_table.end());

  const auto frame_to_free = page_id_to_be_removed->second;
  const auto freed_frame = evict_page(page_id_to_be_removed->first);
  ASSERT(freed_frame == frame_to_free);
  spdlog::info("Evicted frame {} successfully!", frame_to_evict.value());

  const auto frame_to_insert = m_empty_frames.front();
  m_page_table.emplace(id, frame_to_insert);
  m_empty_frames.pop_front();

  ASSERT(m_frames.begin() + freed_frame < m_frames.end());
  m_frames[freed_frame] = frame_header{freed_frame, buffer};
  if (should_pin) m_frames[freed_frame].increase_pin_count();

  m_frame_replacer.recordAccess(frame_to_evict.value());

  return m_frames[freed_frame];
}

template <disk_manager_t T>
bool buffer_pool<T>::flush_pages() {
  bool was_any_page_dirty = false;
  for (const auto &frame : m_frames) {
    if (frame.is_dirty) {
      was_any_page_dirty = true;
      flush_page(frame.frame_id);
    }
  }

  return was_any_page_dirty;
}

template <disk_manager_t T>
bool buffer_pool<T>::flush_page(page_id_t page_id) {
  spdlog::info("Flushing page {}", page_id);
  const auto frame_id_it = m_page_table.find(page_id);
  if (frame_id_it == m_page_table.end()) return false;
  auto &frame = m_frames.at(frame_id_it->second);

  ASSERT(frame.is_dirty);
  auto is_done_promise = std::promise<bool>();
  auto is_done = is_done_promise.get_future();

  disk_request req{.type = disk_request_type::write,
                   .data = frame.get_data(),
                   .page_id = page_id,
                   .is_done = std::move(is_done_promise)};
  m_scheduler.schedule(std::move(req));

  is_done.wait();
  if (!is_done.get())
    throw std::runtime_error("flush_page() failed; tried to write");
  frame.decrease_pin_count();

  const auto frame_to_free = frame.frame_id;
  const auto freed_frame = evict_page(page_id);
  ASSERT(frame_to_free == freed_frame);

  return true;
}

template <disk_manager_t T>
frame_id_t buffer_pool<T>::evict_page(page_id_t page_id) {
  spdlog::info("Evicting page {}", page_id);
  const auto frame_id_it = m_page_table.find(page_id);
  ASSERT(frame_id_it != m_page_table.end());
  const auto frame_id = frame_id_it->second;

  m_frame_replacer.remove(frame_id);
  m_page_table.erase(frame_id_it);

  ASSERT(m_frames[frame_id].frame_id == frame_id);
  m_frames[frame_id] = frame_header{};

  m_empty_frames.push_back(frame_id);

  return frame_id;
}
}  // namespace hivedb
