#pragma once

#include <vector>

namespace hivedb {
    struct real {
        static const std::size_t size = sizeof(float);
        static void serialize(float, std::vector<std::byte>&);
        static float deserialize(std::byte*);
    };
}
