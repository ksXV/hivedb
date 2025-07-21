#pragma once
#include <variant>
#include <cstddef>
#include <map>
#include <vector>
#include <string>

#include <storage_engine/special_types.hpp>
#include <parser/parser.hpp>


namespace hivedb {
    struct table {
        using column_name = std::string;

        std::vector<std::pair<column_name, data_types>> columns;
        std::vector<std::byte> data;
        std::vector<std::unique_ptr<char[]>> strings;
        std::size_t rowSize{0};

        void insertData(std::variant<std::string_view, int, float>);

        [[nodiscard]]
        auto fetch(const std::vector<column_to_fetch>&);

        table() = default;

        table(const table&) = delete;
        table& operator=(const table&) = delete;

        table(table&&) = default;
        table& operator=(table&&) = default;

        ~table() = default;
    };

    struct storage_engine {
        private:
        std::map<std::string, table> tables;

        public:
        void createTable(create_tbl_expr*);
        void insertIntoTable(insert_expr*);
        void queryDataFromTable(select_expr*);
    };
}
