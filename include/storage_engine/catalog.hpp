#pragma once

#include <cstdint>
#include <cstring>
#include <data_types/data_types.hpp>
#include <misc/config.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hivedb {

struct row_key {
  std::int64_t key{0};
  std::int64_t _padding[3]{0};

  constexpr row_key() = default;
  constexpr row_key(std::int64_t k) : key(k) {}  // NOLINT

  [[nodiscard]] constexpr auto operator<=>(const row_key &rhs) const noexcept = default;
  [[nodiscard]] constexpr bool operator!=(const row_key &rhs) const noexcept {
    return key != rhs.key;
  }

  [[nodiscard]] std::string to_string() const { return std::to_string(key); }
  static row_key invalid_key() { return {-99}; }
  static row_key *deserialize(char *buffer) {
    return reinterpret_cast<row_key *>(buffer);
  }
};

static constexpr std::size_t MAX_ROW_DATA_SIZE = 512;

struct row_record {
  std::uint32_t size{0};
  char data[MAX_ROW_DATA_SIZE]{0};

  static row_record *deserialize(char *buffer) {
    return reinterpret_cast<row_record *>(buffer);
  }
  static row_record invalid_key() {
    return row_record{};
  }
};

static constexpr std::uint32_t CATALOG_MAGIC = 0x48495645;  // "HIVE"
static constexpr page_id_t CATALOG_PAGE_ID = 0;
static constexpr std::size_t MAX_CATALOG_TABLES = 4;
static constexpr std::size_t MAX_NAME_LEN = 32;
static constexpr std::size_t MAX_COLUMNS_PER_TABLE = 16;

struct column_meta {
  char name[MAX_NAME_LEN]{0};
  data_types type{data_types::integer};
  bool can_be_null{true};
  std::uint32_t offset{0};
};

struct table_meta {
  char name[MAX_NAME_LEN]{0};
  page_id_t root_page_id{INVALID_PAGE_ID};
  std::uint32_t row_size{0};
  std::uint64_t next_row_id{1};
  std::uint32_t column_count{0};
  column_meta columns[MAX_COLUMNS_PER_TABLE]{};
};

struct catalog_page_layout {
  std::uint32_t magic{CATALOG_MAGIC};
  std::uint32_t table_count{0};
  table_meta tables[MAX_CATALOG_TABLES]{};
};

static_assert(sizeof(catalog_page_layout) <= PAGE_SIZE,
              "Catalog page layout must fit in a single 4096-byte database page");

}  // namespace hivedb
