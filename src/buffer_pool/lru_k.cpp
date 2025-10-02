//
// Created by ksxv on 7/22/25.
//
#include <spdlog/spdlog.h>

#include <algorithm>
#include <buffer_pool/lru_k.hpp>
#include <misc/config.hpp>
#include <numeric>
#include <stdexcept>

namespace hivedb {
lru_k::lru_k(std::int32_t k, frame_id_t size)
    : m_k(k), m_size(size), m_current_nodes(), m_current_timestamp(0) {
  if (k < 0 || size < 0)
    throw std::invalid_argument("k or size must be positive");
}

void lru_k::recordAccess(frame_id_t id) {
  if (id >= m_size)
    throw std::invalid_argument("record_access: id is out of range");

  spdlog::info("Recording access for frame {}", id);
  const auto it = m_current_nodes.find(id);
  if (it == m_current_nodes.end()) {
    std::vector<std::uint64_t> history{};
    history.push_back(m_current_timestamp++);
    m_current_nodes.insert({id, {history, true}});
    return;
  }
  auto &[_, node] = *it;
  node.history.push_back(m_current_timestamp++);
}

void lru_k::setEvictable(frame_id_t id, bool set_evictable) {
  if (id >= m_size)
    throw std::invalid_argument("set_evictable: id is out of range");

  spdlog::info("Setting frame {} to evictable status {}", id, set_evictable);
  const auto it = m_current_nodes.find(id);
  if (it == m_current_nodes.end()) return;

  auto &[_, node] = *it;
  node.is_evictable = set_evictable;
}

std::optional<frame_id_t> lru_k::evict() {
  if (m_current_nodes.empty()) return std::nullopt;

  spdlog::info("Attempting to evict a frame...");
  const auto it = std::find_if(
      m_current_nodes.begin(), m_current_nodes.end(), [this](const auto &i) {
        const auto &[_, node] = i;
        return node.is_evictable &&
               node.history.size() <
                   static_cast<decltype(m_current_nodes)::size_type>(m_k);
      });

  if (it != m_current_nodes.end()) {
    const auto [frame_id, _] = *it;
    m_current_nodes.erase(it);
    spdlog::info("Trivial eviction, evicted {}", frame_id);
    return frame_id;
  }

  std::size_t max_distance = 0;
  frame_id_t id = INVALID_FRAME_ID;

  // time to find a victim, god speed we don't hit a pinned frame
  for (const auto &[frame_id, node] : m_current_nodes) {
    if (!node.is_evictable) continue;
    const auto old_timestamp =
        node.history[node.history.size() + 1 -
                     static_cast<std::vector<uint64_t>::size_type>(m_k)];
    const auto diff = m_current_timestamp - old_timestamp;
    if (diff > max_distance) {
      max_distance = diff;
      id = frame_id;
    }
  }

  // sanity check
  if (id == INVALID_FRAME_ID) {
    spdlog::warn("COULDN'T FIND FRAME TO EVICT!");
    return std::nullopt;
  }

  m_current_nodes.erase(id);

  spdlog::info("Found {}! evicting it now...", id);
  return id;
}
void lru_k::remove(frame_id_t id) {
  if (id >= m_size) throw std::invalid_argument("remove: id is out of range");

  if (const auto it = m_current_nodes.find(id); it != m_current_nodes.end()) {
    const auto &[_, node] = *it;
    if (node.is_evictable) {
      m_current_nodes.erase(id);
      spdlog::info("Removing frame {}", id);
      return;
    }
  }
}

std::size_t lru_k::size() const {
  return std::accumulate(m_current_nodes.begin(), m_current_nodes.end(), 0,
                         [](auto acc, const auto &item) {
                           return acc + item.second.is_evictable;
                         });
}
}  // namespace hivedb
