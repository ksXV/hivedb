#include <cassert>
#include <cstring>
#include <data_types/real.hpp>

namespace hivedb {
void real::serialize(float n, std::vector<std::byte> &buf) {
  const auto *nAsByte = reinterpret_cast<std::byte *>(&n);

  for (std::size_t i = 0; i < sizeof(int); ++i) {
    buf.push_back(nAsByte[i]);
  }
}

float real::deserialize(std::byte *buf) {
  assert(buf != nullptr);

  float n;
  std::memcpy(&n, buf, sizeof(float));

  return n;
}
}  // namespace hivedb
