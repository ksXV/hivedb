#pragma once

#include <parser/tokens.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hivedb {
class lexer {
 private:
  static const std::unordered_map<std::string_view, token_type>
      reservedKeywords;

 private:
  std::string m_input;
  std::size_t m_position;
  std::size_t m_nextPosition;
  char m_current;

  std::vector<token> m_tokens;

  [[nodiscard]] inline bool isAtEnd() const noexcept;
  inline void read() noexcept;
  [[nodiscard]] inline char peek() const noexcept;

  [[nodiscard]] inline bool isLetter(char c);
  [[nodiscard]] inline bool isDigit(char c);

  inline void addToken(token_type, std::string_view) noexcept;

  inline void parseString();

  inline void addIdentifier();
  inline void addNumber();
  inline void nextToken();

 public:
  explicit lexer(std::string_view input);

  std::vector<token> getTokens();
};
}  // namespace hivedb
