#pragma once
#include <filesystem>

namespace hivedb {
struct temporary_file_wrapper {
 private:
  int m_fd;
  std::filesystem::path m_file_path;

 public:
  temporary_file_wrapper();

  [[nodiscard]]
  std::filesystem::path get_path() const;

  temporary_file_wrapper(const temporary_file_wrapper &) = delete;
  temporary_file_wrapper &operator=(const temporary_file_wrapper &) = delete;
  temporary_file_wrapper(temporary_file_wrapper &&) = delete;
  temporary_file_wrapper &operator=(temporary_file_wrapper &&) = delete;

  ~temporary_file_wrapper();
};
}  // namespace hivedb
