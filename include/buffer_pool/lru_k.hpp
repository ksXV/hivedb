#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>
#include <optional>

#include <misc/config.hpp>

namespace hivedb {
	struct lru_k_node {
		std::vector<std::uint64_t> history;
		bool is_evictable;
	};

	struct lru_k {
	private:
		const std::int32_t m_k;
		const std::int64_t m_size;
		std::unordered_map<frame_id_t, lru_k_node> m_current_nodes;
		std::uint64_t m_current_timestamp;
	public:
		lru_k() = delete;
		explicit lru_k(std::int32_t k, frame_id_t size);

		void recordAccess(frame_id_t);
		void setEvictable(frame_id_t, bool);

		[[nodiscard]]
		std::optional<frame_id_t> evict();

		void remove(frame_id_t);
	};
}
