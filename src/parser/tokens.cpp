#include <iostream>
#include <parser/tokens.hpp>
#include <string>
#include <string_view>

namespace hivedb {

token::token(token_type t) : type(t) {}

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
    case token_type::where:
      return "where";
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
    case token_type::substract:
      return "substract";
    case token_type::star:
      return "star";
    case token_type::divide:
      return "divide";
    case token_type::create:
      return "create";
    case token_type::table:
      return "table";
    case token_type::_not:
      return "not";
    case token_type::null:
      return "null";
    case token_type::insert:
      return "insert";
    case token_type::into:
      return "into";
    case token_type::values:
      return "values";
    case token_type::_and:
      return "and";
    case token_type::_or:
      return "or";
    case token_type::equal:
      return "equal";
    case token_type::not_equal:
      return "not_equal";
    case token_type::less:
      return "less";
    case token_type::greater:
      return "greater";
    case token_type::less_equal:
      return "less_equal";
    case token_type::greater_equal:
      return "greater_equal";
    case token_type::integer:
      return "integer";
    case token_type::real:
      return "real";
    default:
      return "unknown";
  }
}

std::ostream &operator<<(std::ostream &os, const token &t) {
  os << "Token: " << getTokenType(t.type) << " | " << t.literal;
  return os;
}

std::string token::name() const {
  return std::string{getTokenType(this->type)};
}
}  // namespace hivedb
