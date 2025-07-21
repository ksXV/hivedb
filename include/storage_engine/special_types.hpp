#pragma once
#include <functional>
#include <data_types/data_types.hpp>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <string>

namespace hivedb {
    struct column_to_fetch {
        std::string_view name;
        std::size_t offset;
        data_types dt;
    };

    struct column_to_fetch_hash {
        std::size_t operator() (const column_to_fetch& v) const {
            return std::hash<std::string_view>()(v.name);
        }
    };

    struct fetched_columns {
        std::byte* ptr;
        data_types dt;
    };

    template<typename ... Bases>
    struct overload_string : Bases ...
    {
        using is_transparent = void;
        using Bases::operator() ... ;
    };


    struct char_pointer_hash
    {
        auto operator()( const char* ptr ) const noexcept
        {
            return std::hash<std::string_view>{}( ptr );
        }
    };

    using transparent_string_hash = overload_string<
        std::hash<std::string>,
        std::hash<std::string_view>,
        char_pointer_hash
    >;

    using fetched_data_map = std::unordered_map<std::string_view, std::vector<fetched_columns>, transparent_string_hash, std::equal_to<>>;
}
