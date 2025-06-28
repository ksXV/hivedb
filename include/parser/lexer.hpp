#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <parser/tokens.hpp>

namespace hivedb {
class Lexer {
    private:
        static const std::unordered_map<std::string_view, TokenType> reservedKeywords;

    private:
        std::string m_input;
        std::size_t m_position;
        std::size_t m_nextPosition;
        char m_current;

        std::vector<Token> m_tokens;

        [[nodiscard]] inline bool isAtEnd() const noexcept;
        inline void read() noexcept;
        [[nodiscard]] inline char peek() const noexcept;

        [[nodiscard]] inline bool isLetter(char c);
        [[nodiscard]] inline bool isDigit(char c);

        inline void addToken(TokenType, std::string_view) noexcept;

        inline void addIdentifier();
        inline void addNumber();
        inline void nextToken();
    public:
        explicit Lexer(std::string_view input);

        std::vector<Token> getTokens();

};
}
