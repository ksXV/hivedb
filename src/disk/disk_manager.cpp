#include <spdlog/spdlog.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <disk/disk_manager.hpp>
#include <filesystem>
#include <misc/config.hpp>
#include <stdexcept>
#include <string>

namespace hivedb {

disk_manager::disk_manager(const std::filesystem::path &db_path)
    : m_db_file(), m_db_file_path(db_path) {
  m_db_file.open(db_path, std::ios::binary | std::ios::in | std::ios::out);
  if (!m_db_file.is_open()) {
    // create the file if opening failed
    m_db_file.open(db_path, std::ios::trunc | std::ios::binary | std::ios::in |
                                std::ios::out);
    if (!m_db_file.is_open())
      throw std::runtime_error("failed create the file!");
  }

  // the first page (or the + 1) is reserved for the system/root of the b+tree
  std::filesystem::resize_file(db_path, (m_current_pages + 1) * PAGE_SIZE);
}

void disk_manager::write_page(page_id_t id, const char *buffer) {
  if (id < 0)
    throw std::runtime_error("Invalid id detected!: " + std::to_string(id));
  offset_t offset =
      m_pages.find(id) != m_pages.end() ? allocate_new_page() : m_pages[id];

  m_db_file.seekg(static_cast<std::fstream::off_type>(offset));
  m_db_file.write(buffer, PAGE_SIZE);

  if (m_db_file.bad()) {
    throw std::runtime_error("Error writing page_id:" + std::to_string(id));
  }

  m_pages[id] = offset;

  m_db_file.flush();
}

void disk_manager::read_page(page_id_t id, char *buffer) {
  if (id < 0)
    throw std::runtime_error("Invalid id detected!: " + std::to_string(id));

  offset_t offset =
      m_pages.find(id) == m_pages.end() ? allocate_new_page() : m_pages[id];

  const auto file_size = get_file_size();
  if (offset > file_size) {
    throw std::runtime_error(
        "Offset was bigger (somehow) than file size! offset: " +
        std::to_string(offset) + " file_size: " + std::to_string(file_size));
  }

  m_pages[id] = offset;

  m_db_file.seekg(static_cast<std::fstream::off_type>(offset));
  m_db_file.read(buffer, PAGE_SIZE);

  if (m_db_file.bad()) {
    throw std::runtime_error("Error reading page_id:" + std::to_string(id));
  }

  const auto read_count = m_db_file.gcount();
  if (PAGE_SIZE > read_count) {
    spdlog::info("warning: couldn't read full page! read: {}", read_count);
    std::memset(buffer + read_count, 0, PAGE_SIZE - read_count);
  }
}

void disk_manager::delete_page(page_id_t id) {
  if (id < 0)
    throw std::runtime_error("Invalid id detected!: " + std::to_string(id));
  if (m_pages.find(id) == m_pages.end())
    throw std::runtime_error("Invalid id detected!: " + std::to_string(id));

  offset_t offset = m_pages[id];

  m_db_file.seekg(static_cast<std::fstream::off_type>(offset));
  m_db_file.write(nullptr, PAGE_SIZE);

  m_free_offsets.push_back(offset);
  m_pages.erase(id);
}

offset_t disk_manager::allocate_new_page() {
  if (!m_free_offsets.empty()) {
    auto offset = m_free_offsets.back();
    m_free_offsets.pop_back();
    return offset;
  }

  if (m_pages.size() + 1 >= m_current_pages) {
    m_current_pages *= 2;
    std::filesystem::resize_file(m_db_file_path,
                                 (m_current_pages + 1) * PAGE_SIZE);
  }

  return m_pages.size() * PAGE_SIZE;
}
std::size_t disk_manager::get_file_size() {
  struct stat st{};

  int status = stat(m_db_file_path.c_str(), &st);
  if (status == -1) {
    throw std::runtime_error("Failed to fetch the file size! ERRNO: " +
                             std::to_string(errno));
  }

  return st.st_size;
}
}  // namespace hivedb
