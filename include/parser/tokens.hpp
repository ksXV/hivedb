#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

#include <string_view>

namespace hivedb {
enum struct token_type {
  select,
  from,

  create,
  table,
  _not,
  null,

  insert,
  into,
  values,

  parenthesesL,
  parenthesesR,

  add,
  substract,
  star,
  divide,
  bang,

  quote,
  comma,
  dot,

  identifier,
  string,
  integer,
  real,

  eof,

  illegal,
};

struct token {
  token_type type{};
  std::string_view literal{};

  token() = default;
  explicit token(token_type t);
  token(token_type t, std::string_view l);

  friend bool operator==(const token &, const token &) noexcept;

  friend std::ostream &operator<<(std::ostream &os, const token &t);

  [[nodiscard]] std::string name() const;
};
}  // namespace hivedb

// for fmt::format
template <>
struct fmt::formatter<hivedb::token> : formatter<std::string> {
  auto format(hivedb::token t, format_context &ctx) const {
    return formatter<std::string>::format(t.name(), ctx);
  }
};
