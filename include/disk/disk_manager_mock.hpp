#pragma once
#include <array>
#include <filesystem>
#include <misc/config.hpp>
#include <vector>

namespace hivedb {
struct disk_manager_mock {
 private:
  using mock_page = std::array<char, PAGE_SIZE>;
  std::vector<mock_page> m_mock_file;

 public:
  explicit disk_manager_mock(const std::filesystem::path &);

  void read_page(page_id_t, char *);
  void write_page(page_id_t, const char *);
  void delete_page(page_id_t);
};
}  // namespace hivedb
