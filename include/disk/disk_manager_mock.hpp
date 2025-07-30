#pragma once
#include <filesystem>
#include <vector>
#include <array>

#include <misc/config.hpp>
#include <buffer_pool/buffer_pool.hpp>

namespace hivedb {
    struct disk_manager_mock {
        private:
        using mock_page = std::array<char, PAGE_SIZE>;
        std::vector<mock_page> m_mock_file;

        public:
        explicit disk_manager_mock(const std::filesystem::path&);

        void read_page(page_id_t, char*);
        void write_page(page_id_t, const char*);
        void delete_page(page_id_t);
    };
}
