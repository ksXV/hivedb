//
// Created by ksxv on 7/22/25.
//

#include <buffer_pool/buffer_pool.hpp>
#include <misc/config.hpp>

#include "buffer_pool/lru_k.hpp"

namespace hivedb {
frame_header::frame_header(frame_id_t id, std::vector<char> &data,
                           lru_k *replacer)
    : frame_id(id),
      is_dirty(false),
      m_pin_count(0),
      m_data(std::move(data)),
      m_replacer(replacer) {}
void frame_header::increase_pin_count() { m_pin_count += 1; }
void frame_header::decrease_pin_count() { m_pin_count -= 1; }
std::int32_t frame_header::get_pin_count() const { return m_pin_count; }
const char *frame_header::get_data() const { return m_data.data(); }
char *frame_header::get_data() { return m_data.data(); }
frame_header::~frame_header() {
  if (m_replacer) {
    m_replacer->setEvictable(frame_id, true);
  }
}
}  // namespace hivedb
