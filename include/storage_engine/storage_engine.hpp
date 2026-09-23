#pragma once

#include <parser/parser.hpp>
#include <b_plus_tree/b_plus_tree.hpp>
#include <buffer_pool/buffer_pool.hpp>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <misc/temporary_file_wrapper.hpp>
#include <optional>
#include <storage_engine/catalog.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace hivedb {

struct query_result {
  std::vector<std::string> column_names;
  std::vector<std::vector<exprs::values>> rows;
};

struct storage_engine {
 private:
  std::unique_ptr<temporary_file_wrapper> m_temp_file{nullptr};
  std::filesystem::path m_db_path;
  buffer_pool<disk_manager> m_bp;
  std::vector<std::unique_ptr<char[]>> m_strings;

  void initCatalog();
  [[nodiscard]] std::optional<table_meta> findTable(std::string_view name);
  void saveTableMeta(const table_meta &meta);

 public:
  explicit storage_engine(const std::filesystem::path &db_path,
                          std::size_t max_frames = 64);
  storage_engine();

  storage_engine(const storage_engine &) = delete;
  storage_engine &operator=(const storage_engine &) = delete;

  storage_engine(storage_engine &&) = default;
  storage_engine &operator=(storage_engine &&) = default;

  ~storage_engine() = default;

  void createTable(create_tbl_expr *);
  void insertIntoTable(insert_expr *);
  [[nodiscard]] query_result executeSelect(select_expr *);
  void queryDataFromTable(select_expr *);
};

}  // namespace hivedb
