//
// Created by ksxv on 7/22/25.
//

#include <buffer_pool/buffer_pool.hpp>
#include <misc/config.hpp>

namespace hivedb {
frame_header::frame_header(frame_id_t id, std::vector<char> &data)
    : frame_id(id), is_dirty(false), m_pin_count(0), m_data(std::move(data)) {}
void frame_header::increase_pin_count() { m_pin_count += 1; }
void frame_header::decrease_pin_count() { m_pin_count -= 1; }
std::int32_t frame_header::get_pin_count() const { return m_pin_count; }
const char *frame_header::get_data() const { return m_data.data(); }
char *frame_header::get_data() { return m_data.data(); }
}  // namespace hivedb
