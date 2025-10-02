#include <cstring>
#include <disk/disk_manager_mock.hpp>
#include <misc/config.hpp>

namespace hivedb {
disk_manager_mock::disk_manager_mock(const std::filesystem::path &) {}

void disk_manager_mock::read_page(page_id_t id, char *buffer) {
  if (id < 0) {
    return;
  }

  if (static_cast<decltype(m_mock_file)::size_type>(id) >= m_mock_file.size()) {
    m_mock_file.resize(m_mock_file.size() + 6);
  }

  auto &page = m_mock_file[id];
  std::memcpy(buffer, &page[0], PAGE_SIZE);
};

void disk_manager_mock::write_page(page_id_t id, const char *buffer) {
  if (id < 0) {
    return;
  }

  if (static_cast<decltype(m_mock_file)::size_type>(id) >= m_mock_file.size()) {
    m_mock_file.resize(m_mock_file.size() + 6);
  }

  auto &page = m_mock_file[id];
  std::memcpy(&page[0], buffer, PAGE_SIZE);
};

void delete_page(page_id_t){};
}  // namespace hivedb
