#pragma once

#include <string>
#include <string_view>

#include <parser/tokens.h>

class Lexer {
    private:
        const std::string input;
        std::size_t position;
        std::size_t readPosition;
        char currentChar;

        void readChar() noexcept;
        void skipWhiteSpaces() noexcept;
        Token getNextToken(TokenType t) const noexcept;
        std::string_view getWord() noexcept;
        std::string_view getIdentifier() noexcept;
    public:
        Lexer(const std::string& i) noexcept:
            input{i}, position{0}, readPosition{1}, currentChar{} {};
        Lexer(std::string_view i) noexcept:
            input{i}, position{0}, readPosition{1}, currentChar{} {};

        Token nextToken();
};
