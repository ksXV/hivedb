#include <cassert>
#include <cmath>
#include <memory>
#include <parser/parser.hpp>
#include <parser/tokens.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace hivedb {
parser::parser(const std::vector<token> &t) : m_tokens(t) {}

const token &parser::current() const noexcept {
  assert(m_cursor <= m_tokens.size());
  return m_tokens[m_cursor];
}

const token &parser::previous() const noexcept {
  assert(m_cursor != 0);
  return m_tokens[m_cursor - 1];
}

const token &parser::peek() const noexcept {
  assert(m_cursor + 1 <= m_tokens.size());
  return m_tokens[m_cursor + 1];
}

bool parser::isDone() const noexcept { return m_tokens.size() <= m_cursor; }

const token &parser::advance() noexcept {
  if (!isDone()) m_cursor++;
  return previous();
}

bool parser::check(token_type type) const noexcept {
  if (isDone()) return false;
  return current().type == type;
}

template <std::same_as<token_type>... T>
bool parser::match(T... types) noexcept {
  return (... || check(types)) && (advance(), true);
}

void parser::consume(token_type type, std::string_view err) {
  if (match(type)) return;
  throw std::invalid_argument(err.data());
}

std::unique_ptr<exprs> parser::binaryExpr() {
  auto e = unaryExpr();
  if (match(token_type::add, token_type::substract, token_type::divide,
            token_type::star)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(e);
    b->op = previous().type;
    b->rhs = binaryExpr();

    return b;
  }

  return e;
}

std::unique_ptr<exprs> parser::unaryExpr() {
  if (match(token_type::substract, token_type::bang)) {
    auto e = std::make_unique<unary_expr>();
    e->op = previous().type;
    e->rhs = unaryExpr();
    return e;
  }

  return primaryExpr();
}

std::unique_ptr<exprs> parser::primaryExpr() {
  if (match(token_type::string)) {
    const auto literal = previous().literal;

    auto e = std::make_unique<literal_expr<std::string>>(
        std::string{literal.data(), literal.size()}, false);

    return e;
  }

  if (match(token_type::identifier)) {
    const auto literal = previous().literal;

    auto e = std::make_unique<literal_expr<std::string>>(
        std::string{literal.data(), literal.size()}, true);

    return e;
  }

  if (match(token_type::integer)) {
    const auto literal = previous().literal;

    // TODO: remove this dumb allocation...
    auto e =
        std::make_unique<literal_expr<int>>(std::stoi(literal.data()), false);
    return e;
  }

  if (match(token_type::real)) {
    const auto literal = previous().literal;

    // TODO: also remove this dumb allocation...
    auto e =
        std::make_unique<literal_expr<float>>(std::stof(literal.data()), false);
    return e;
  }

  if (match(token_type::parenthesesL)) {
    auto e = match(token_type::select) ? selectExpr() : binaryExpr();

    consume(token_type::parenthesesR, "Missing parentheses.");

    auto g = std::make_unique<grouping_expr>();
    g->expr = std::move(e);
    return g;
  }

  if (match(token_type::select)) {
    auto e = selectExpr();

    auto g = std::make_unique<grouping_expr>();
    g->expr = std::move(e);
    return g;
  }

  throw std::invalid_argument("Invalid token: " +
                              std::string(current().name()));
}

std::unique_ptr<exprs> parser::expr() {
  if (match(token_type::select)) {
    return selectExpr();
  } else if (match(token_type::create)) {
    return createExpr();
  } else if (match(token_type::insert)) {
    return insertExpr();
  }
  throw std::invalid_argument("Invalid token: " +
                              std::string(current().name()));
}

std::unique_ptr<exprs> parser::insertExpr() {
  auto e = std::make_unique<insert_expr>();
  consume(token_type::into, "Invalid insert stmt! Missing INTO.");

  consume(token_type::identifier,
          "Invalid insert stmt! Where do i insert into?");
  e->tblName = previous().literal;

  consume(token_type::parenthesesL, "Invalid insert stmt! Missing \"(\" !");

  while (!isDone()) {
    consume(token_type::identifier, "Invalid insert stmt! Missing column!");
    std::string_view column = previous().literal;

    e->columns.push_back(column);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    throw std::invalid_argument("SHOULDNT REACH THIS!");
  }

  consume(token_type::values, "Invalid insert stmt! Missing values!!!");

  consume(token_type::parenthesesL,
          "Invalid insert stmt! Missing values \"(\" !");

  while (!isDone()) {
    std::variant<std::string_view, int, float> value;
    if (match(token_type::string)) {
      value = previous().literal;
    } else if (match(token_type::integer)) {
      value = std::stoi(std::string(previous().literal));
    } else if (match(token_type::real)) {
      value = std::stof(std::string(previous().literal));
    } else {
      throw std::invalid_argument("Unimplemented data type!");
    }

    e->values.emplace_back(value);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    throw std::invalid_argument("SHOULDNT REACH THIS!");
  }

  return e;
}

std::unique_ptr<exprs> parser::createExpr() {
  auto e = std::make_unique<create_tbl_expr>();

  consume(token_type::table, "Invalid create stmt! Missing TABLE.");

  consume(token_type::identifier,
          "Invalid create stmt! Missing identifier for table name.");

  e->tblName = previous().literal;

  consume(token_type::parenthesesL,
          "Invalid create stmt! Missing parantheses!");

  while (!isDone()) {
    consume(token_type::identifier,
            "Invalid create stmt! missing column name!");
    std::string_view tblName = previous().literal;

    consume(token_type::identifier,
            "Invalid create stmt! missing type for column: " +
                std::string(tblName));
    std::string_view type = previous().literal;

    bool canBeNull = false;
    if (match(token_type::_not)) {
      consume(token_type::null,
              "Invalid create stmt! Wtf are you trying to negate.");
      canBeNull = true;
    }

    e->tblColumns.emplace_back(tblName, type, canBeNull);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    throw std::invalid_argument("SHOULDNT REACH THIS!");
  }
  return e;
}

std::unique_ptr<exprs> parser::selectExpr() {
  auto s = std::make_unique<select_expr>();
  if (match(token_type::parenthesesL)) {
    auto e = binaryExpr();
    s->innerExpr.push_back(std::move(e));

    while (match(token_type::comma)) {
      e = binaryExpr();
      s->innerExpr.push_back(std::move(e));
    }
    consume(token_type::parenthesesR, "INVALID SELECT STMT");
  } else {
    auto e = binaryExpr();
    s->innerExpr.push_back(std::move(e));
  }

  if (match(token_type::from)) {
    consume(token_type::identifier,
            "Invalid select stmt detected! Invalid token detected");

    s->tblName = previous().literal;
  }

  // TODO: continue for where here
  return s;
}

std::unique_ptr<exprs> parser::parse() {
  std::unique_ptr<exprs> e = expr();

  return e;
}
}  // namespace hivedb
