#pragma once

#include <cstdint>
#include <map>
#include <misc/config.hpp>
#include <optional>
#include <vector>

namespace hivedb {
struct lru_k_node {
  std::vector<std::uint64_t> history;
  bool is_evictable;
};

struct lru_k {
 private:
  const std::int32_t m_k;
  const std::int64_t m_size;
  std::map<frame_id_t, lru_k_node> m_current_nodes;
  std::uint64_t m_current_timestamp;

 public:
  explicit lru_k(std::int32_t k, frame_id_t size);

  lru_k() = delete;
  lru_k(const lru_k &) = delete;
  lru_k &operator=(const lru_k &) = delete;
  lru_k(lru_k &&) = delete;
  lru_k &operator=(lru_k &&) = delete;
  ~lru_k() = default;

  void recordAccess(frame_id_t);
  void setEvictable(frame_id_t, bool);

  [[nodiscard]]
  std::optional<frame_id_t> evict();

  void remove(frame_id_t);

  [[nodiscard]]
  std::size_t size() const;
};
}  // namespace hivedb
