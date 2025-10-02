#include <iostream>
#include <parser/tokens.hpp>

namespace hivedb {

token::token(token_type t) : type(t), literal() {}
token::token(token_type t, std::string_view l) : type(t), literal(l) {}

bool operator==(const token &lhs, const token &rhs) noexcept {
  return lhs.literal == rhs.literal && lhs.type == rhs.type;
}

static constexpr std::string_view getTokenType(token_type t) {
  switch (t) {
    case token_type::illegal:
      return "illegal";
    case token_type::eof:
      return "eof";
    case token_type::parenthesesL:
      return "parenthesesL";
    case token_type::parenthesesR:
      return "parenthesesR";
    case token_type::add:
      return "add";
    case token_type::select:
      return "select";
    case token_type::from:
      return "from";
    case token_type::identifier:
      return "identifier";
    case token_type::quote:
      return "quote";
    case token_type::bang:
      return "bang";
    case token_type::comma:
      return "comma";
    case token_type::dot:
      return "dot";
    case token_type::string:
      return "string";
    default:
      return "unknown yet";
  }
}

std::ostream &operator<<(std::ostream &os, const token &t) {
  os << "{ ";
  if (!t.literal.empty()) {
    os << t.literal << " ";
  }
  os << getTokenType(t.type);
  os << " }";
  return os;
}

std::string token::name() const {
  if (literal.empty()) {
    return std::string{getTokenType(type)};
  }

  return std::string{literal} + " " + std::string{getTokenType(type)};
}
}  // namespace hivedb
