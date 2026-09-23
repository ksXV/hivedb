#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <data_types/data_types.hpp>
#include <data_types/integer.hpp>
#include <data_types/real.hpp>
#include <data_types/varchar.hpp>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <storage_engine/storage_engine.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <b_plus_tree/b_plus_tree.hpp>
#include <storage_engine/catalog.hpp>

namespace hivedb {

storage_engine::storage_engine(const std::filesystem::path &db_path,
                               std::size_t max_frames)
    : m_db_path(db_path), m_bp(static_cast<frame_id_t>(max_frames), m_db_path) {
  initCatalog();
}

storage_engine::storage_engine()
    : m_temp_file(std::make_unique<temporary_file_wrapper>()),
      m_db_path(m_temp_file->get_path()),
      m_bp(64, m_db_path) {
  initCatalog();
}

void storage_engine::initCatalog() {
  // Ensure catalog page (page 0) is reserved so subsequent allocate_new_page() calls
  // for table B+ trees allocate page 1 and higher.
  static_cast<void>(m_bp.allocate_new_page());

  std::error_code ec;
  auto fsize = std::filesystem::file_size(m_db_path, ec);
  if (ec || fsize < PAGE_SIZE) {
    auto &frame = m_bp.request_page(CATALOG_PAGE_ID, true);
    auto *bytes = frame.get_data();
    std::fill_n(bytes, PAGE_SIZE, '\0');
    auto *layout = reinterpret_cast<catalog_page_layout *>(bytes);
    layout->magic = CATALOG_MAGIC;
    layout->table_count = 0;
    frame.is_dirty = true;
    frame.decrease_pin_count();
    m_bp.flush_page(CATALOG_PAGE_ID, false);
  } else {
    auto &frame = m_bp.request_page(CATALOG_PAGE_ID, false);
    auto *bytes = frame.get_data();
    auto *layout = reinterpret_cast<catalog_page_layout *>(bytes);
    if (layout->magic != CATALOG_MAGIC) {
      std::fill_n(bytes, PAGE_SIZE, '\0');
      layout->magic = CATALOG_MAGIC;
      layout->table_count = 0;
      frame.is_dirty = true;
      m_bp.flush_page(CATALOG_PAGE_ID, false);
    }
    frame.decrease_pin_count();
  }
}

std::optional<table_meta> storage_engine::findTable(std::string_view name) {
  auto &frame = m_bp.request_page(CATALOG_PAGE_ID, false);
  auto *layout = reinterpret_cast<catalog_page_layout *>(frame.get_data());
  for (std::uint32_t i = 0; i < layout->table_count; ++i) {
    if (std::string_view(layout->tables[i].name) == name) {
      table_meta copy = layout->tables[i];
      frame.decrease_pin_count();
      return copy;
    }
  }
  frame.decrease_pin_count();
  return std::nullopt;
}

void storage_engine::saveTableMeta(const table_meta &meta) {
  auto &frame = m_bp.request_page(CATALOG_PAGE_ID, true);
  auto *layout = reinterpret_cast<catalog_page_layout *>(frame.get_data());
  for (std::uint32_t i = 0; i < layout->table_count; ++i) {
    if (std::string_view(layout->tables[i].name) == meta.name) {
      layout->tables[i] = meta;
      break;
    }
  }
  frame.is_dirty = true;
  frame.decrease_pin_count();
  m_bp.flush_page(CATALOG_PAGE_ID, false);
}

void storage_engine::createTable(create_tbl_expr *expr) {
  assert(expr != nullptr);

  auto existing = findTable(expr->tblName);
  if (existing.has_value()) {
    throw std::runtime_error("Table already exists: " +
                             std::string(expr->tblName));
  }

  table_meta meta{};
  std::strncpy(meta.name, expr->tblName.data(),
               std::min(expr->tblName.size(), MAX_NAME_LEN - 1));
  meta.root_page_id = INVALID_PAGE_ID;
  meta.next_row_id = 1;
  meta.row_size = 0;
  meta.column_count = 0;

  for (const auto &column : expr->tblColumns) {
    if (!isTypeValid(column.type)) {
      throw std::invalid_argument("Invalid type '" + std::string(column.type) +
                                  "' detected for column '" + std::string(column.name) + "'");
    }
    if (meta.column_count >= MAX_COLUMNS_PER_TABLE) {
      throw std::runtime_error("Table '" + std::string(expr->tblName) + "' exceeds maximum allowed columns (" + std::to_string(MAX_COLUMNS_PER_TABLE) + ")");
    }
    column_meta &cm = meta.columns[meta.column_count++];
    std::strncpy(cm.name, column.name.data(),
                 std::min(column.name.size(), MAX_NAME_LEN - 1));
    cm.type = fromString(column.type);
    cm.can_be_null = column.can_be_null;
    cm.offset = meta.row_size;
    meta.row_size += findOffset(cm.type);
  }

  auto &frame = m_bp.request_page(CATALOG_PAGE_ID, true);
  auto *layout = reinterpret_cast<catalog_page_layout *>(frame.get_data());
  if (layout->table_count >= MAX_CATALOG_TABLES) {
    frame.decrease_pin_count();
    throw std::runtime_error("Catalog limit reached: database cannot store more than " + std::to_string(MAX_CATALOG_TABLES) + " tables");
  }

  layout->tables[layout->table_count++] = meta;
  frame.is_dirty = true;
  frame.decrease_pin_count();
  m_bp.flush_page(CATALOG_PAGE_ID, false);

  std::cout << "OK! Created table with name " << expr->tblName << "\n";
}

void storage_engine::insertIntoTable(insert_expr *expr) {
  assert(expr != nullptr);

  auto meta_opt = findTable(expr->tblName);
  if (!meta_opt.has_value()) {
    throw std::runtime_error("Table '" + std::string(expr->tblName) + "' does not exist");
  }
  table_meta meta = *meta_opt;

  for (const auto &exprColumn : expr->columns) {
    bool found = false;
    for (std::uint32_t i = 0; i < meta.column_count; ++i) {
      if (std::string_view(meta.columns[i].name) == exprColumn) {
        found = true;
        break;
      }
    }
    if (!found) {
      throw std::runtime_error("Column '" + std::string(exprColumn) + "' does not exist in table '" + std::string(expr->tblName) + "'");
    }
  }

  row_record rec{};
  rec.size = meta.row_size;

  std::vector<std::byte> row_bytes(meta.row_size, std::byte{0});

  for (std::uint32_t c = 0; c < meta.column_count; ++c) {
    const auto &cm = meta.columns[c];
    std::size_t valIdx = SIZE_MAX;
    for (std::size_t i = 0; i < expr->columns.size(); ++i) {
      if (expr->columns[i] == std::string_view(cm.name)) {
        valIdx = i;
        break;
      }
    }

    if (valIdx != SIZE_MAX) {
      const auto &val = expr->values[valIdx];
      std::vector<std::byte> col_bytes;
      std::visit(
          [this, &col_bytes, &cm](auto &&v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<int, T>) {
              if (cm.type == data_types::integer) {
                integer::serialize(v, col_bytes);
              } else if (cm.type == data_types::real) {
                real::serialize(static_cast<float>(v), col_bytes);
              }
            } else if constexpr (std::is_same_v<float, T>) {
              if (cm.type == data_types::real) {
                real::serialize(v, col_bytes);
              } else if (cm.type == data_types::integer) {
                integer::serialize(static_cast<int>(v), col_bytes);
              }
            } else if constexpr (std::is_same_v<std::string_view, T>) {
              if (cm.type == data_types::varchar) {
                varchar::serialize(v, col_bytes, m_strings);
              }
            }
          },
          val);

      if (!col_bytes.empty()) {
        std::size_t copy_size = std::min(col_bytes.size(), static_cast<std::size_t>(meta.row_size - cm.offset));
        std::memcpy(row_bytes.data() + cm.offset, col_bytes.data(), copy_size);
      }
    }
  }

  if (meta.row_size <= MAX_ROW_DATA_SIZE) {
    std::memcpy(rec.data, row_bytes.data(), meta.row_size);
  }

  b_plus_tree<disk_manager, row_key, row_key, row_record> tree{
      meta.root_page_id, m_bp};

  row_key key{static_cast<std::int64_t>(meta.next_row_id++)};
  static_cast<void>(tree.insert(key, rec));

  meta.root_page_id = tree.get_root_page_id();
  saveTableMeta(meta);
}

query_result storage_engine::executeSelect(select_expr *expr) {
  assert(expr != nullptr);
  query_result res;

  if (expr->tblName.empty()) {
    if (expr->whereExpr != nullptr) {
      auto cond = expr->whereExpr->execute();
      bool pass = std::visit(
          overload{
              [](int v) { return v != 0; },
              [](float v) { return v != 0.0f; },
              [](std::string_view v) { return !v.empty() && v != "0"; },
              [](const std::vector<exprs::values> &vec) {
                return !vec.empty();
              }},
          cond);
      if (!pass) return res;
    }

    auto value = expr->execute();
    std::vector<exprs::values> row;
    std::visit(
        overload{
            [&row](int v) { row.emplace_back(v); },
            [&row](float v) { row.emplace_back(v); },
            [&row](std::string_view v) { row.emplace_back(v); },
            [&row](const std::vector<exprs::values> &vec) {
              for (const auto &v : vec) row.push_back(v);
            }},
        value);
    res.rows.push_back(std::move(row));
    return res;
  }

  auto meta_opt = findTable(expr->tblName);
  if (!meta_opt.has_value()) {
    throw std::runtime_error("Table '" + std::string(expr->tblName) + "' does not exist");
  }
  table_meta meta = *meta_opt;

  // Validate that all referenced columns exist in table schema
  auto referenced_cols = expr->retriveColumnsToBeFetched();
  for (const auto &col : referenced_cols) {
    if (col == "*") continue;
    bool found = false;
    for (std::uint32_t i = 0; i < meta.column_count; ++i) {
      if (std::string_view(meta.columns[i].name) == col) {
        found = true;
        break;
      }
    }
    if (!found) {
      throw std::runtime_error("Column '" + std::string(col) +
                               "' does not exist in table '" +
                               std::string(expr->tblName) + "'");
    }
  }

  if (expr->hasWildcard() && expr->innerExpr.size() == 1) {
    for (std::uint32_t i = 0; i < meta.column_count; ++i) {
      res.column_names.emplace_back(meta.columns[i].name);
    }
  } else {
    for (const auto &ie : expr->innerExpr) {
      if (dynamic_cast<const wildcard_expr *>(ie.get()) != nullptr) {
        for (std::uint32_t i = 0; i < meta.column_count; ++i) {
          res.column_names.emplace_back(meta.columns[i].name);
        }
      } else if (auto *lit = dynamic_cast<const literal_expr<std::string> *>(ie.get());
                 lit != nullptr && lit->isIdentifier) {
        res.column_names.push_back(lit->value);
      } else {
        std::stringstream ss;
        ie->prettyPrint(ss);
        res.column_names.push_back(ss.str());
      }
    }
  }

  if (meta.root_page_id == INVALID_PAGE_ID) {
    return res;
  }

  b_plus_tree<disk_manager, row_key, row_key, row_record> tree{
      meta.root_page_id, m_bp};

  tree.scan([&](const row_key & /*k*/, const row_record &rec) {
    fetched_data_map rowData;
    for (std::uint32_t c = 0; c < meta.column_count; ++c) {
      const auto &cm = meta.columns[c];
      std::byte *ptr = reinterpret_cast<std::byte *>(
          const_cast<char *>(rec.data + cm.offset));
      rowData[cm.name].push_back({ptr, cm.type});
    }

    if (expr->whereExpr != nullptr) {
      auto cond = expr->whereExpr->execute(rowData, 0);
      bool pass = std::visit(
          overload{
              [](int v) { return v != 0; },
              [](float v) { return v != 0.0f; },
              [](std::string_view v) { return !v.empty() && v != "0"; },
              [](const std::vector<exprs::values> &vec) {
                if (vec.empty()) return false;
                return std::visit(
                    overload{
                        [](int v) { return v != 0; },
                        [](float v) { return v != 0.0f; },
                        [](std::string_view v) {
                          return !v.empty() && v != "0";
                        },
                        [](const auto &) { return true; }},
                    vec[0]);
              }},
          cond);
      if (!pass) return;
    }

    std::vector<exprs::values> row;
    if (expr->hasWildcard() && expr->innerExpr.size() == 1) {
      for (std::uint32_t c = 0; c < meta.column_count; ++c) {
        const auto &cm = meta.columns[c];
        std::byte *ptr = reinterpret_cast<std::byte *>(
            const_cast<char *>(rec.data + cm.offset));
        switch (cm.type) {
          case data_types::integer:
            row.emplace_back(integer::deserialize(ptr));
            break;
          case data_types::real:
            row.emplace_back(real::deserialize(ptr));
            break;
          case data_types::varchar:
            row.emplace_back(varchar::deserialize(ptr));
            break;
        }
      }
    } else {
      for (const auto &ie : expr->innerExpr) {
        if (dynamic_cast<const wildcard_expr *>(ie.get()) != nullptr) {
          for (std::uint32_t c = 0; c < meta.column_count; ++c) {
            const auto &cm = meta.columns[c];
            std::byte *ptr = reinterpret_cast<std::byte *>(
                const_cast<char *>(rec.data + cm.offset));
            switch (cm.type) {
              case data_types::integer:
                row.emplace_back(integer::deserialize(ptr));
                break;
              case data_types::real:
                row.emplace_back(real::deserialize(ptr));
                break;
              case data_types::varchar:
                row.emplace_back(varchar::deserialize(ptr));
                break;
            }
          }
        } else {
          auto val = ie->execute(rowData, 0);
          std::visit(
              overload{
                  [&row](int v) { row.emplace_back(v); },
                  [&row](float v) { row.emplace_back(v); },
                  [&row](std::string_view v) { row.emplace_back(v); },
                  [&row](const std::vector<exprs::values> &vec) {
                    for (const auto &v : vec) row.push_back(v);
                  }},
              val);
        }
      }
    }

    res.rows.push_back(std::move(row));
  });

  return res;
}

void storage_engine::queryDataFromTable(select_expr *expr) {
  auto res = executeSelect(expr);
  for (std::size_t i = 0; i < res.column_names.size(); ++i) {
    std::cout << res.column_names[i]
              << (i + 1 == res.column_names.size() ? "" : ", ");
  }
  std::cout << "\n--------------------------------\n";
  for (const auto &row : res.rows) {
    for (std::size_t j = 0; j < row.size(); ++j) {
      std::visit(overload{[](int v) { std::cout << v; },
                          [](float v) { std::cout << v; },
                          [](std::string_view v) { std::cout << v; }},
                 row[j]);
      if (j + 1 < row.size()) std::cout << ", ";
    }
    std::cout << "\n";
  }
}

}  // namespace hivedb