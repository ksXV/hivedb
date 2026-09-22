#include <cassert>
#include <cmath>
#include <fmt/format.h>
#include <memory>
#include <parser/parser.hpp>
#include <parser/tokens.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace hivedb {

static std::string formatToken(const token &t) {
  if (t.literal.empty()) {
    return fmt::format("'{}'", token_type_name(t.type));
  }
  return fmt::format("'{}' ({})", t.literal, token_type_name(t.type));
}

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

  const std::string actual = isDone() ? "end of input" : formatToken(current());
  throw std::invalid_argument(fmt::format(
      "Syntax error: {}. Expected '{}', but found {} at token position {}.",
      err, token_type_name(type), actual, m_cursor));
}

std::unique_ptr<exprs> parser::binaryExpr() {
  return logicalOrExpr();
}

std::unique_ptr<exprs> parser::logicalOrExpr() {
  auto expr = logicalAndExpr();
  while (match(token_type::_or)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(expr);
    b->op = previous().type;
    b->rhs = logicalAndExpr();
    expr = std::move(b);
  }
  return expr;
}

std::unique_ptr<exprs> parser::logicalAndExpr() {
  auto expr = comparisonExpr();
  while (match(token_type::_and)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(expr);
    b->op = previous().type;
    b->rhs = comparisonExpr();
    expr = std::move(b);
  }
  return expr;
}

std::unique_ptr<exprs> parser::comparisonExpr() {
  auto expr = additiveExpr();
  while (match(token_type::equal, token_type::not_equal,
               token_type::less, token_type::less_equal,
               token_type::greater, token_type::greater_equal)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(expr);
    b->op = previous().type;
    b->rhs = additiveExpr();
    expr = std::move(b);
  }
  return expr;
}

std::unique_ptr<exprs> parser::additiveExpr() {
  auto expr = multiplicativeExpr();
  while (match(token_type::add, token_type::substract)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(expr);
    b->op = previous().type;
    b->rhs = multiplicativeExpr();
    expr = std::move(b);
  }
  return expr;
}

std::unique_ptr<exprs> parser::multiplicativeExpr() {
  auto expr = unaryExpr();
  while (match(token_type::star, token_type::divide)) {
    auto b = std::make_unique<binary_expr>();
    b->lhs = std::move(expr);
    b->op = previous().type;
    b->rhs = unaryExpr();
    expr = std::move(b);
  }
  return expr;
}

std::unique_ptr<exprs> parser::unaryExpr() {
  if (match(token_type::substract, token_type::bang, token_type::_not)) {
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

    auto e =
        std::make_unique<literal_expr<int>>(std::stoi(literal.data()), false);
    return e;
  }

  if (match(token_type::real)) {
    const auto literal = previous().literal;

    auto e =
        std::make_unique<literal_expr<float>>(std::stof(literal.data()), false);
    return e;
  }

  if (match(token_type::parenthesesL)) {
    auto e = match(token_type::select) ? selectExpr() : binaryExpr();

    consume(token_type::parenthesesR, "Unclosed '(' in expression");

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

  const std::string actual = isDone() ? "end of input" : formatToken(current());
  throw std::invalid_argument(fmt::format(
      "Syntax error: unexpected {} at token position {}: expected expression (literal value, column identifier, subquery, or '(')",
      actual, m_cursor));
}

std::unique_ptr<exprs> parser::expr() {
  if (match(token_type::select)) {
    return selectExpr();
  } else if (match(token_type::create)) {
    return createExpr();
  } else if (match(token_type::insert)) {
    return insertExpr();
  }
  const std::string actual = isDone() ? "end of input" : formatToken(current());
  throw std::invalid_argument(fmt::format(
      "Syntax error: unexpected {} at token position {}: expected statement starting with SELECT, CREATE TABLE, or INSERT INTO",
      actual, m_cursor));
}

std::unique_ptr<exprs> parser::insertExpr() {
  auto e = std::make_unique<insert_expr>();
  consume(token_type::into, "Expected 'INTO' keyword after 'INSERT'");

  consume(token_type::identifier,
          "Expected target table name after 'INSERT INTO'");
  e->tblName = previous().literal;

  consume(token_type::parenthesesL,
          "Expected '(' before column list in INSERT statement");

  while (!isDone()) {
    consume(token_type::identifier,
            "Expected column name in INSERT statement");
    std::string_view column = previous().literal;

    e->columns.push_back(column);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    const std::string actual = isDone() ? "end of input" : formatToken(current());
    throw std::invalid_argument(fmt::format(
        "Syntax error in INSERT statement: expected ',' or ')' after column '{}', but found {} at token position {}.",
        column, actual, m_cursor));
  }

  consume(token_type::values, "Expected 'VALUES' keyword in INSERT statement");

  consume(token_type::parenthesesL,
          "Expected '(' before values list in INSERT statement");

  while (!isDone()) {
    std::variant<std::string_view, int, float> value;
    if (match(token_type::string)) {
      value = previous().literal;
    } else if (match(token_type::integer)) {
      value = std::stoi(std::string(previous().literal));
    } else if (match(token_type::real)) {
      value = std::stof(std::string(previous().literal));
    } else {
      const std::string actual = isDone() ? "end of input" : formatToken(current());
      throw std::invalid_argument(fmt::format(
          "Syntax error in INSERT statement: expected literal value (string, integer, or real), but found {} at token position {}.",
          actual, m_cursor));
    }

    e->values.emplace_back(value);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    const std::string actual = isDone() ? "end of input" : formatToken(current());
    throw std::invalid_argument(fmt::format(
        "Syntax error in INSERT statement: expected ',' or ')' after value in VALUES list, but found {} at token position {}.",
        actual, m_cursor));
  }

  return e;
}

std::unique_ptr<exprs> parser::createExpr() {
  auto e = std::make_unique<create_tbl_expr>();

  consume(token_type::table, "Expected 'TABLE' keyword after 'CREATE'");

  consume(token_type::identifier,
          "Expected table name identifier after 'CREATE TABLE'");

  e->tblName = previous().literal;

  consume(token_type::parenthesesL,
          "Expected '(' before column definitions in CREATE TABLE");

  while (!isDone()) {
    consume(token_type::identifier,
            "Expected column name identifier in CREATE TABLE");
    std::string_view colName = previous().literal;

    consume(token_type::identifier,
            fmt::format("Expected data type for column '{}' in CREATE TABLE",
                        colName));
    std::string_view type = previous().literal;

    bool canBeNull = true;
    if (match(token_type::_not)) {
      consume(token_type::null,
              fmt::format("Expected 'NULL' after 'NOT' for column '{}' in CREATE TABLE",
                          colName));
      canBeNull = false;
    }

    e->tblColumns.emplace_back(colName, type, canBeNull);

    if (match(token_type::comma)) {
      continue;
    }
    if (match(token_type::parenthesesR)) {
      break;
    }

    const std::string actual = isDone() ? "end of input" : formatToken(current());
    throw std::invalid_argument(fmt::format(
        "Syntax error in CREATE TABLE: expected ',' or ')' after definition of column '{}', but found {} at token position {}.",
        colName, actual, m_cursor));
  }
  return e;
}

std::unique_ptr<exprs> parser::selectExpr() {
  auto s = std::make_unique<select_expr>();

  if (match(token_type::parenthesesL)) {
    do {
      if (match(token_type::star)) {
        s->innerExpr.push_back(std::make_unique<wildcard_expr>());
      } else if (match(token_type::select)) {
        auto sub = selectExpr();
        auto g = std::make_unique<grouping_expr>();
        g->expr = std::move(sub);
        s->innerExpr.push_back(std::move(g));
      } else {
        s->innerExpr.push_back(binaryExpr());
      }
    } while (match(token_type::comma));

    consume(token_type::parenthesesR,
            "Expected closing ')' after projection list in SELECT statement");
  } else {
    do {
      if (match(token_type::star)) {
        s->innerExpr.push_back(std::make_unique<wildcard_expr>());
      } else if (match(token_type::select)) {
        auto sub = selectExpr();
        auto g = std::make_unique<grouping_expr>();
        g->expr = std::move(sub);
        s->innerExpr.push_back(std::move(g));
      } else {
        s->innerExpr.push_back(binaryExpr());
      }
    } while (match(token_type::comma));
  }

  if (match(token_type::from)) {
    consume(token_type::identifier,
            "Expected table name identifier after 'FROM' in SELECT statement");

    s->tblName = previous().literal;
  }

  if (match(token_type::where)) {
    s->whereExpr = binaryExpr();
  }

  return s;
}

std::unique_ptr<exprs> parser::parse() {
  std::unique_ptr<exprs> e = expr();

  if (match(token_type::eof)) {
    // optional eof / ';'
  }

  if (!isDone()) {
    const std::string actual = formatToken(current());
    throw std::invalid_argument(fmt::format(
        "Syntax error: unexpected extra token {} at token position {}. Expected end of query.",
        actual, m_cursor));
  }

  return e;
}

}  // namespace hivedb
