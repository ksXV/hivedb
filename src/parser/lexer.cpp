#include <cassert>
#include <cctype>
#include <cstdio>
#include <parser/lexer.hpp>
#include <parser/tokens.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hivedb {

using namespace std::literals;
const std::unordered_map<std::string_view, token_type> lexer::reservedKeywords{
    {"select"sv, token_type::select}, {"from"sv, token_type::from},
    {"where"sv, token_type::where},
    {"create"sv, token_type::create}, {"table"sv, token_type::table},
    {"not"sv, token_type::_not},      {"null"sv, token_type::null},
    {"insert"sv, token_type::insert}, {"into"sv, token_type::into},
    {"values"sv, token_type::values},
    {"and"sv, token_type::_and},      {"or"sv, token_type::_or},
};

lexer::lexer(std::string_view input)
    : m_input{input}, m_position{0}, m_nextPosition{1}, m_current{} {
  if (input.empty()) {
    throw std::invalid_argument("Input query cannot be empty");
  }
  m_current = input[m_position];
};

bool lexer::isLetter(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

bool lexer::isDigit(char c) { return (c >= '0' && c <= '9'); }

bool lexer::isAtEnd() const noexcept {
  if (m_nextPosition > m_input.length()) return true;
  return false;
}

void lexer::read() noexcept {
  m_current = isAtEnd() ? '\0' : m_input[m_nextPosition];
  m_position = m_nextPosition++;
}

char lexer::peek() const noexcept {
  if (isAtEnd()) return '\0';
  return m_input[m_nextPosition];
}

void lexer::addToken(token_type type,
                     std::string_view identifier) noexcept {
  m_tokens.emplace_back(type, identifier);
}

void lexer::nextToken() {
  assert(!m_input.empty());

  switch (m_current) {
    case '(':
      addToken(token_type::parenthesesL);
      break;
    case ')':
      addToken(token_type::parenthesesR);
      break;
    case '+':
      addToken(token_type::add);
      break;
    case '-':
      addToken(token_type::substract);
      break;
    case '/':
      addToken(token_type::divide);
      break;
    case '*':
      addToken(token_type::star);
      break;
    case ',':
      addToken(token_type::comma);
      break;
    case '.':
      addToken(token_type::dot);
      break;
    case '=':
      if (peek() == '=') {
        const std::size_t start = m_position;
        read();
        addToken(token_type::equal,
                 std::string_view{m_input.c_str() + start, 2});
      } else {
        addToken(token_type::equal, "=");
      }
      break;
    case '!':
      if (peek() == '=') {
        const std::size_t start = m_position;
        read();
        addToken(token_type::not_equal,
                 std::string_view{m_input.c_str() + start, 2});
      } else {
        addToken(token_type::bang, "!");
      }
      break;
    case '<':
      if (peek() == '=') {
        const std::size_t start = m_position;
        read();
        addToken(token_type::less_equal,
                 std::string_view{m_input.c_str() + start, 2});
      } else if (peek() == '>') {
        const std::size_t start = m_position;
        read();
        addToken(token_type::not_equal,
                 std::string_view{m_input.c_str() + start, 2});
      } else {
        addToken(token_type::less, "<");
      }
      break;
    case '>':
      if (peek() == '=') {
        const std::size_t start = m_position;
        read();
        addToken(token_type::greater_equal,
                 std::string_view{m_input.c_str() + start, 2});
      } else {
        addToken(token_type::greater, ">");
      }
      break;
    case '\0':
      [[fallthrough]];
    case ';':
      addToken(token_type::eof);
      break;
    case '\'':
      [[fallthrough]];
    case '\"':
      parseString();
      break;

    case ' ':
      [[fallthrough]];
    case '\n':
      [[fallthrough]];
    case '\r':
      [[fallthrough]];
    case '\t':
      break;

    default:
      if (isLetter(m_current)) {
        addIdentifier();
      } else if (isDigit(m_current)) {
        addNumber();
      } else {
        throw std::invalid_argument(std::string("Unrecognized character '") + m_current + "' at position " + std::to_string(m_position));
      }
      break;
  }

  read();
}

void lexer::addNumber() {
  const std::size_t currentPos = m_position;
  while (isDigit(peek())) read();

  if (peek() == '.' && (m_nextPosition + 1 < m_input.length() && isDigit(m_input[m_nextPosition + 1]))) {
    read();
    while (isDigit(peek())) read();

    const std::string_view number{m_input.c_str() + currentPos,
                                  m_position - currentPos + 1};

    addToken(token_type::real, number);
    return;
  }

  const std::string_view number{m_input.c_str() + currentPos,
                                m_position - currentPos + 1};

  addToken(token_type::integer, number);
}

void lexer::parseString() {
  const char quote = m_current;
  read();

  const std::size_t currentPos = m_position;

  while (!isAtEnd() && peek() != quote) {
    read();
  }
  const std::string_view str{m_input.c_str() + currentPos,
                             m_position - currentPos + 1};

  // skip the second "
  read();

  addToken(token_type::string, str);
}

void lexer::addIdentifier() {
  const std::size_t currentPos = m_position;
  while (isLetter(peek()) || isDigit(peek())) {
    read();
  }

  const std::string_view keyword{m_input.c_str() + currentPos,
                                 m_position - currentPos + 1};

  std::string lowerKeyword;
  lowerKeyword.reserve(keyword.size());
  for (char c : keyword) {
    lowerKeyword.push_back(
        static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }

  if (auto it = reservedKeywords.find(lowerKeyword);
      it != reservedKeywords.end()) {
    const auto [i, t] = *it;
    addToken(t, keyword);
    return;
  }

  addToken(token_type::identifier, keyword);
}

std::vector<token> lexer::getTokens() {
  while (m_current != '\0') {
    nextToken();
  }

  return m_tokens;
}
}  // namespace hivedb
