//
// Created by Pam Bondi herself on 7/22/25.
//
#pragma once
#include <disk/disk_manager.hpp>
#include <filesystem>
#include <future>
#include <misc/channel.hpp>
#include <misc/config.hpp>
#include <thread>
#include <type_traits>

namespace hivedb {
template <typename T>
concept disk_manager_t =
    requires(T manager, page_id_t id, char *buffer, const char *c_buffer) {
      manager.read_page(id, buffer);
      manager.write_page(id, c_buffer);
      manager.delete_page(id);
    } && std::is_constructible_v<T, const std::filesystem::path &>;

enum struct disk_request_type {
  write,
  read,

  // shutdown the thread
  shutdown,
};

struct disk_request {
  disk_request_type type;
  char *data;
  page_id_t page_id;
  std::promise<bool> is_done;
};

template <disk_manager_t T>
struct disk_scheduler {
 private:
  T m_manager;
  std::thread m_worker_thread;
  channel<disk_request> m_requests_queue;

 public:
  explicit disk_scheduler(const std::filesystem::path &);

  void schedule(disk_request &&);

  disk_scheduler(const disk_scheduler &) = delete;
  disk_scheduler &operator=(const disk_scheduler &) = delete;
  disk_scheduler(const disk_scheduler &&) = delete;
  disk_scheduler &operator=(const disk_scheduler &&) = delete;

  ~disk_scheduler();
};

// put these up there
template <disk_manager_t T>
disk_scheduler<T>::disk_scheduler(const std::filesystem::path &db_path)
    : m_manager(db_path), m_requests_queue() {
  m_worker_thread = std::thread([this]() {
    while (true) {
      disk_request req = m_requests_queue.get();
      switch (req.type) {
        case disk_request_type::read:
          m_manager.read_page(req.page_id, req.data);
          req.is_done.set_value(true);
          break;
        case disk_request_type::write:
          m_manager.write_page(req.page_id, req.data);
          req.is_done.set_value(true);
          break;
        case disk_request_type::shutdown:
          return;
        default:
          throw std::invalid_argument("invalid request type");
      }
    }
  });
}

template <disk_manager_t T>
void disk_scheduler<T>::schedule(disk_request &&req) {
  m_requests_queue.put(std::move(req));
}

template <disk_manager_t T>
disk_scheduler<T>::~disk_scheduler() {
  m_requests_queue.put(
      disk_request{disk_request_type::shutdown, nullptr, 0, {}});
  if (m_worker_thread.joinable()) {
    m_worker_thread.join();
  }
}
}  // namespace hivedb
