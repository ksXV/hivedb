#include <sys/types.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <data_types/varchar.hpp>
#include <memory>
#include <span>
#include <string>

namespace hivedb {
void varchar::serialize(std::string_view str, std::vector<std::byte> &buf,
                        std::vector<std::unique_ptr<char[]>> &varchars) {
  // TODO: add overflow check later
  auto strSize = static_cast<uint16_t>(str.size());

  const auto *sizeAsBytes = reinterpret_cast<std::byte *>(&strSize);
  buf.push_back(sizeAsBytes[0]);
  buf.push_back(sizeAsBytes[1]);

  auto ptr = std::make_unique<char[]>(strSize);
  std::memcpy(ptr.get(), &str[0], strSize);

  const auto actualPtr = ptr.get();
  auto ptrAsBytes = std::as_bytes(std::span{&actualPtr, 1});

  buf.insert(buf.cend(), ptrAsBytes.begin(), ptrAsBytes.end());

  varchars.push_back(std::move(ptr));
}

std::string_view varchar::deserialize(std::byte *buf) {
  assert(buf != nullptr);

  uint16_t strSize;
  std::memcpy(&strSize, buf, sizeof(uint16_t));

  const char *ptr{};
  std::memcpy(&ptr, (buf + sizeof(uint16_t)), sizeof(ptr));

  std::string_view str{ptr, strSize};

  return str;
}

}  // namespace hivedb
