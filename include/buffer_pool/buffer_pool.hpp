#pragma once

#include <unordered_map>
#include <vector>
#include <cstdint>
#include <list>

#include <misc/config.hpp>
#include <disk/disk_scheduler.hpp>
#include <buffer_pool/lru_k.hpp>

namespace hivedb {
    struct frame_header {
        frame_id_t frame_id;
        bool is_dirty;
    private:
        std::int32_t m_pin_count;
        std::vector<char> m_data;

    public:
        frame_header() = delete;
        explicit frame_header(frame_id_t, std::vector<char>&);

        frame_header(const frame_header&) = delete;
        frame_header& operator=(const frame_header&) = delete;

        frame_header(frame_header&&) = default;
        frame_header& operator=(frame_header&&) = default;
        ~frame_header() = default;

        void increase_pin_count();
        void decrease_pin_count();

        [[nodiscard]]
        std::int32_t get_pin_count() const;

        [[nodiscard]]
        const char* get_data() const;

        [[nodiscard]]
        char* get_data();
    };

    template<disk_manager_t T>
    struct buffer_pool {
    public:
        const frame_id_t max_frames;
    private:
        std::unordered_map<page_id_t, frame_id_t> m_page_table;
        std::vector<frame_header> m_frames;
        std::list<frame_id_t> m_empty_frames;

        disk_scheduler<T> m_scheduler;
        lru_k m_frame_replacer;

        static constexpr auto m_k = 10;
        frame_id_t m_next_frame;
        bool check_for_unpinned_frames();
    public:
        buffer_pool() = delete;
        explicit buffer_pool(frame_id_t);

        [[nodiscard]]
        frame_header& request_page(page_id_t);

        frame_id_t evict_page(page_id_t);
        bool flush_page(page_id_t);
        bool flush_pages();
    };
}
