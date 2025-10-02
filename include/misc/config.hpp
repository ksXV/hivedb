#pragma once

#include <cstdint>

namespace hivedb {
using page_id_t = std::int64_t;
using offset_t = std::size_t;
using frame_id_t = std::int64_t;

static constexpr std::int32_t PAGE_SIZE = 4096;
static constexpr frame_id_t INVALID_FRAME_ID = -1;
static constexpr page_id_t INVALID_PAGE_ID = -1;
}  // namespace hivedb
