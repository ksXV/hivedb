//
// Created by ksxv on 7/24/25.
//
#pragma once

#include <misc/config.hpp>
#include <disk/disk_scheduler.hpp>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace hivedb {
   static constexpr std::size_t DEFAULT_NUMBER_OF_PAGES = 16;

   struct disk_manager {
       private:
       std::size_t m_current_pages{DEFAULT_NUMBER_OF_PAGES};
       std::unordered_map<page_id_t, offset_t> m_pages;
       std::vector<offset_t> m_free_offsets;

       std::fstream m_db_file;
       std::filesystem::path m_db_file_path;

       [[nodiscard]]
       offset_t allocate_new_page();

       [[nodiscard]]
       std::size_t get_file_size();

       public:
       explicit disk_manager(const std::filesystem::path&);

       void read_page(page_id_t, char*);
       void write_page(page_id_t, const char*);
       void delete_page(page_id_t);
   };
}
