#pragma once

#include <vector>

namespace hivedb {
    struct integer {
        static const std::size_t size = sizeof(int);
        static void serialize(int, std::vector<std::byte>&);
        static int deserialize(std::byte*);
    };
}
