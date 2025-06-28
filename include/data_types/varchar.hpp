#pragma once

#include <string>

namespace hivedb {
    struct varchar {
        std::string data;

        explicit varchar(std::string_view str);

        varchar(const varchar&) = delete;
        varchar& operator=(const varchar&) = delete;

        varchar(varchar&& rhs) = default;
        varchar& operator=(varchar&& rhs) = default;

        ~varchar() = default;
    };
}
