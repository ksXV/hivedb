//
// Created by ksxv on 7/22/25.
//
#include <algorithm>
#include <stdexcept>
#include <buffer_pool/lru_k.hpp>

namespace hivedb {
    lru_k::lru_k(std::int32_t k, frame_id_t size): m_k(k), m_size(size), m_current_nodes(), m_current_timestamp(0) {
       if (k < 0 || size < 0) throw std::invalid_argument("k or size must be positive");
    }

    void lru_k::recordAccess(frame_id_t id) {
        if (id >= m_size) throw std::invalid_argument("id is out of range");

        const auto it = m_current_nodes.find(id);
        if (it == m_current_nodes.end()) {
            std::vector<std::uint64_t> history{};
            history.push_back(m_current_timestamp++);
            m_current_nodes.emplace(id, history);
            return;
        }
        auto& [_, node] = *it;
        node.history.push_back(m_current_timestamp++);
    }

    void lru_k::setEvictable(frame_id_t id, bool set_evictable) {
        if (id >= m_size) throw std::invalid_argument("id is out of range");

        const auto it = m_current_nodes.find(id);
        if (it == m_current_nodes.end()) return;

        auto& [_, node] = *it;
        node.is_evictable = set_evictable;
    }

    [[nodiscard]]
    std::optional<frame_id_t> lru_k::evict() {
        const auto it = std::find_if(m_current_nodes.begin(), m_current_nodes.end(), [this](const auto& i) {
            const auto& [_, node] = i;
            return node.is_evictable && node.history.size() < m_k;
        });

        if (it != m_current_nodes.end()) {
            const auto& [frame_id, _] = *it;
            m_current_nodes.erase(it);
            return frame_id;
        }

        size_t max_distance = 0;
        frame_id_t id = -1;

        for (const auto& [frame_id, node] : m_current_nodes) {
            if (!node.is_evictable) {continue;}
            const auto timestamp = node.history[node.history.size() - static_cast<std::vector<uint64_t>::size_type>(m_k) + 1];
            const auto diff = m_current_timestamp - timestamp;
            if (diff > max_distance) {
                max_distance = diff;
                id = frame_id;
            }
        }

        //sanity check
        if (id == -1) {
            return std::nullopt;
        }

        m_current_nodes.erase(id);
        return id;
    }
    void lru_k::remove(frame_id_t id) {
        if (id >= m_size) throw std::invalid_argument("id is out of range");

        if (const auto it = m_current_nodes.find(id); it != m_current_nodes.end()) {
            const auto& [_, node] = *it;
            if (node.is_evictable) {
                m_current_nodes.erase(id);
                return;
            }
        }
    }

}