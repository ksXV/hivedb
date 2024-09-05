#pragma once

#include <string_view>
#include <variant>

enum class TokenType {
    illegal,
    eof,
    parenthesesL,
    parenthesesR,
    plusOperator,
    quoationMark,
    select,
    from,
    identifier
};

class Token {
    private:
        TokenType type;
        std::variant<std::string_view, char> literal;

    public:
        constexpr Token(TokenType t, std::string_view l) noexcept : type{t}, literal{l} {};
            Token(TokenType t, char l) : type{t}, literal{l} {};
        Token() noexcept {};

        bool operator==(const Token& other) const noexcept {
            return other.literal == this->literal && other.type == this->type;
        }

};

std::ostream& operator<<(std::ostream& os, const Token& t);
