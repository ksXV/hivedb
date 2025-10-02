#include <unistd.h>

#include <misc/temporary_file_wrapper.hpp>
#include <stdexcept>

namespace hivedb {
temporary_file_wrapper::temporary_file_wrapper() {
  char temp_template[] = "/tmp/i_hate_linux_apis_and_cppXXXXXXXX";

  m_fd = mkstemp(temp_template);
  if (m_fd == -1) {
    throw std::runtime_error("FAILED TO CREATE TEMP FILE");
  }

  m_file_path = std::filesystem::path{temp_template};
}

[[nodiscard]]
std::filesystem::path temporary_file_wrapper::get_path() const {
  return m_file_path;
}

temporary_file_wrapper::~temporary_file_wrapper() {
  close(m_fd);
  unlink(m_file_path.c_str());
}
}  // namespace hivedb
