#pragma once

#include <memory>
#include <string_view>
#include <vector>

namespace hivedb {
struct varchar {
  static const std::size_t size = 2 + sizeof(const char *);
  static void serialize(std::string_view, std::vector<std::byte> &,
                        std::vector<std::unique_ptr<char[]>> &);
  static std::string_view deserialize(std::byte *);
};
}  // namespace hivedb
