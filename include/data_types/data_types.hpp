#pragma once
#include <string_view>

namespace hivedb {
enum struct data_types {
  varchar,
  integer,
  real,
};

std::string_view toString(data_types dt);
data_types fromString(std::string_view dt);
bool isTypeValid(std::string_view dt);
std::size_t findOffset(data_types dt);
}  // namespace hivedb
