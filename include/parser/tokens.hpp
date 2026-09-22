#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

#include <string_view>

namespace hivedb {
enum struct token_type {
  select,
  from,
  where,

  create,
  table,
  _not,
  null,

  insert,
  into,
  values,

  _and,
  _or,

  parenthesesL,
  parenthesesR,

  equal,
  not_equal,
  less,
  greater,
  less_equal,
  greater_equal,

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

constexpr std::string_view token_type_name(token_type t) noexcept {
  switch (t) {
    case token_type::select:        return "select";
    case token_type::from:          return "from";
    case token_type::where:         return "where";
    case token_type::create:        return "create";
    case token_type::table:         return "table";
    case token_type::_not:          return "not";
    case token_type::null:          return "null";
    case token_type::insert:        return "insert";
    case token_type::into:          return "into";
    case token_type::values:        return "values";
    case token_type::_and:          return "and";
    case token_type::_or:           return "or";
    case token_type::parenthesesL:  return "(";
    case token_type::parenthesesR:  return ")";
    case token_type::equal:         return "=";
    case token_type::not_equal:     return "!=";
    case token_type::less:          return "<";
    case token_type::greater:       return ">";
    case token_type::less_equal:    return "<=";
    case token_type::greater_equal: return ">=";
    case token_type::add:           return "+";
    case token_type::substract:     return "-";
    case token_type::star:          return "*";
    case token_type::divide:        return "/";
    case token_type::bang:          return "!";
    case token_type::quote:         return "\"";
    case token_type::comma:         return ",";
    case token_type::dot:           return ".";
    case token_type::identifier:    return "identifier";
    case token_type::string:        return "string";
    case token_type::integer:       return "integer";
    case token_type::real:          return "real";
    case token_type::eof:           return "eof";
    case token_type::illegal:       return "illegal";
  }
  return "unknown";
}

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
