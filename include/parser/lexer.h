#pragma once

#include <exception>
#include <string>
#include <string_view>

#include "tokens.h"

class Lexer {
    private:
        const std::string input;
        std::size_t position;
        std::size_t readPosition;
        char currentChar;
    public:
        Lexer(const std::string& i):
        input{i}, position{0}, readPosition{1}, currentChar{input.at(0)} { if (i.empty()) { throw std::bad_exception(); }};
        Lexer(std::string_view i):
        input{i}, position{0}, readPosition{1}, currentChar{input.at(0)} { if (i.empty()) { throw std::bad_exception(); }};

        void readChar() noexcept;
        void skipWhiteSpaces() noexcept;
        Token nextToken() noexcept;
        Token getNextToken(TokenType t) const noexcept;
        std::string_view getWord() noexcept;
};
