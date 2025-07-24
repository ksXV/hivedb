#pragma once

#include <unordered_map>
#include <cstddef>
#include <vector>
#include <cstdint>
#include <list>

#include <disk/disk_scheduler.hpp>
#include <buffer_pool/lru_k.hpp>

namespace hivedb {
    static constexpr std::size_t MAX_PAGE_SIZE = 4096;

    using page_id_t = std::int64_t;
    using frame_id_t = std::int64_t;

    struct frame_header {
        const frame_id_t frame_id;
        bool is_dirty;
    private:
        std::int32_t m_pin_count;

        std::vector<std::byte> m_data;

    public:
        frame_header() = delete;
        explicit frame_header(frame_id_t, std::vector<std::byte>&);

        frame_header(const frame_header&) = delete;
        frame_header& operator=(const frame_header&) = delete;

        frame_header(frame_header&&) = default;
        frame_header& operator=(frame_header&&) = delete;
        ~frame_header() = default;

        void increase_pin_count();
        void decrease_pin_count();
        std::int32_t get_pin_count() const;

        const std::byte* get_data() const;
        std::byte* get_data();
    };

    struct buffer_pool {

    public:
        const frame_id_t max_frames;
    private:
        std::unordered_map<page_id_t, frame_id_t> m_page_table;
        std::vector<frame_header> m_frames;
        std::list<frame_id_t> m_empty_frames;

        disk_scheduler m_scheduler;
        lru_k m_frame_replacer;

        constexpr auto m_k = 10;
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
