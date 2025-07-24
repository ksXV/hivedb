//
// Created by ksxv on 7/22/25.
//
#include <algorithm>
#include <stdexcept>
#include <cassert>
#include <numeric>

#include <buffer_pool/buffer_pool.hpp>

namespace hivedb {
    frame_header::frame_header(frame_id_t id, std::vector<std::byte>& data)
    :frame_id(id), is_dirty(false), m_pin_count(0), m_data(std::move(data)) {}

    void frame_header::increase_pin_count() {
        m_pin_count += 1;
    }

    void frame_header::decrease_pin_count() {
        m_pin_count -= 1;
    }
    std::int32_t frame_header::get_pin_count() const {
       return m_pin_count;
    }
    const std::byte* frame_header::get_data() const {
        return m_data.data();
    }
    std::byte* frame_header::get_data() {
        return m_data.data();
    }

    buffer_pool::buffer_pool(frame_id_t max_frames): max_frames(max_frames), m_frame_replacer(m_k, max_frames)  {
        if (max_frames < 0) throw std::invalid_argument("max_frames must be >= 0");

        m_frames.reserve(max_frames);
        for (frame_id_t i = 0; i < max_frames; i++) {
            m_empty_frames.push_back(i);
        }
    }


    bool buffer_pool::check_for_unpinned_frames() {
        return std::find_if(m_frames.begin(), m_frames.end(), [](const frame_header& frame) {
            return frame.get_pin_count() == 0;
        }) != m_frames.end();
    }

    frame_header& buffer_pool::request_page(page_id_t id) {
        if (const auto it = m_page_table.find(id); it != m_page_table.end()) {
            //found our page yippie
            m_frame_replacer.recordAccess(it->second);
            m_frame_replacer.setEvictable(it->second, false);

            auto& frame = m_frames.at(it->second);
            frame.increase_pin_count();

            return frame;
        }

        if (!check_for_unpinned_frames()) throw std::invalid_argument("OOM! all frames are pinned!");

        //page fault :(
        //first get the page from the disk
        std::vector<std::byte> buffer;
        buffer.reserve(MAX_PAGE_SIZE);
        std::promise<bool> is_done_promise;
        auto is_done = is_done_promise.get_future();

        disk_request req{
            .type = disk_request_type::read,
            .page_id = id,
            .data = buffer.data(),
            .is_done = std::move(is_done_promise)
        };
        m_scheduler.schedule(req);

        is_done.wait();
        if (!is_done.get()) throw std::runtime_error("request_page() failed");

        //do we have an empty space in the page table?
        if (m_page_table.size() < max_frames) {
            assert(!m_empty_frames.empty());
            auto frame_id = m_empty_frames.front();
            m_page_table.emplace(id, frame_id);
            m_empty_frames.pop_front();

            m_frames.emplace(m_frames.begin() + frame_id, id, buffer);
            m_frame_replacer.recordAccess(frame_id);

            auto& frame = m_frames[frame_id];
            frame.increase_pin_count();
            return frame;
        }

        //we don't :(
        //maybe scan for a dirty page and write it
        auto it = std::ranges::find_if(m_frames, [](const frame_header& frame) -> bool {
            return frame.is_dirty;
        });

        if (it != m_frames.end()) {
            frame_id_t frame_id = std::distance(m_frames.begin(), it);

            const auto page_id_to_be_flushed_it = std::ranges::find_if(m_page_table, [frame_id](const auto& pair) {
                return pair->second == frame_id;
            });

            assert(page_id_to_be_flushed_it != m_page_table.end());

            bool status = flush_page(page_id_to_be_flushed_it->first);
            if (!status) throw std::runtime_error("flush_page() failed");

            frame_id = m_empty_frames.front();
            m_page_table.emplace(id, frame_id);
            m_empty_frames.pop_front();

            m_frame_replacer.recordAccess(frame_id);

            const auto inserted_frame = m_frames.emplace(m_frames.begin() + frame_id, frame_id, buffer);
            inserted_frame->increase_pin_count();

            m_frame_replacer.recordAccess(frame_id);

            return *inserted_frame;
        }

        //no dirty page :(
        //time to find a victim
        auto frame_to_evict = m_frame_replacer.evict();
        if (!frame_to_evict.has_value()) throw std::runtime_error("frame_replacer.evict() failed; got -1 or invalid frame");
        if (m_frames.at(frame_to_evict.value()).get_pin_count() > 0) throw std::runtime_error("frame_to_evict() failed; got a pinned frame");

        const auto page_id_to_be_removed = std::ranges::find_if(m_page_table, [frame_to_evict](const auto& pair) {
            return pair->second == frame_to_evict.value();
        });

        assert(page_id_to_be_removed != m_page_table.end());
        evict_page(page_id_to_be_removed->first);
        auto frame_to_insert = m_empty_frames.front();
        m_page_table.emplace(id,  frame_to_insert);
        m_empty_frames.pop_front();

        const auto inserted_frame = m_frames.emplace(m_frames.begin() + frame_to_insert, frame_to_insert,buffer);
        inserted_frame->increase_pin_count();

        m_frame_replacer.recordAccess(frame_to_evict.value());

        return *inserted_frame;
    }

    bool buffer_pool::flush_pages() {
        bool was_any_page_dirty = false;
        for (const auto& frame : m_frames) {
            if (frame.is_dirty) {
                was_any_page_dirty = true;
                flush_page(frame.frame_id);
            }
        }

        return was_any_page_dirty;
    }

    bool buffer_pool::flush_page(page_id_t page_id) {
        const auto frame_id_it = m_page_table.find(page_id);
        if (frame_id_it == m_page_table.end()) return false;
        auto& frame = m_frames.at(frame_id_it->second);

        assert(frame.is_dirty);
        auto is_done_promise = std::promise<bool>();
        auto is_done = is_done_promise.get_future();

        disk_request req{
            .type = disk_request_type::write,
            .page_id = page_id,
            .data = frame.get_data(),
            .is_done = std::move(is_done_promise)
        };
        m_scheduler.schedule(req);

        is_done.wait();
        if (!is_done.get()) throw std::runtime_error("request_page() failed; tried to write");

        evict_page(page_id);

        return true;
    }

    frame_id_t buffer_pool::evict_page(page_id_t page_id) {
        const auto frame_id_it = m_page_table.find(page_id);
        assert(frame_id_it != m_page_table.end());
        const auto frame_id = frame_id_it->second;

        if(m_frames[frame_id].get_pin_count() != 0) throw std::invalid_argument("oom: victim frame is pinned!");

        m_frame_replacer.remove(frame_id);
        m_page_table.erase(frame_id_it);

        m_frames.erase(m_frames.begin() + frame_id);
        m_empty_frames.push_back(frame_id);

        return frame_id;
    }
}