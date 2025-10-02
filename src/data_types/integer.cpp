#include <cassert>
#include <cstring>
#include <data_types/integer.hpp>

namespace hivedb {
void integer::serialize(int n, std::vector<std::byte> &buf) {
  const auto *nAsByte = reinterpret_cast<std::byte *>(&n);
  for (std::size_t i = 0; i < sizeof(int); ++i) {
    buf.push_back(nAsByte[i]);
  }
}

int integer::deserialize(std::byte *buf) {
  assert(buf != nullptr);

  int n;
  std::memcpy(&n, buf, sizeof(int));

  return n;
}
}  // namespace hivedb
