#pragma once

#include <string_view>
#include <variant>

#include <fmt/core.h>
#include <fmt/format.h>

enum class TokenType {
    illegal,
    eof,
    parenthesesL,
    parenthesesR,
    plusOperator,
    select,
    from,
    identifier
};

class Token {
    private:
        TokenType type;
        std::variant<std::string_view, char> literal;

    public:
        constexpr Token(TokenType t, std::string_view l) noexcept : type{t}, literal{l} {
            if (l.empty()) {
                literal = "<empty>";
            }
        };
        constexpr Token(TokenType t, char l) : type{t}, literal{l} {
            if (!l) {
                literal = "<empty>";
            }
        };
        Token() noexcept {};

        bool operator==(const Token& other) const noexcept {
            return other.literal == this->literal && other.type == this->type;
        }
        constexpr TokenType getTokenType() const {
            return type;
        }
        constexpr std::variant<std::string_view, char> getLiteral() const {
            return literal;
        }

        friend std::ostream& operator<<(std::ostream& os, const Token& t);

        virtual std::string name() const;
};

//for fmt::format
template <>
struct fmt::formatter<Token> : formatter<std::string> {
    auto format(Token t, format_context& ctx) const {
        return formatter<std::string>::format(t.name(), ctx);
    }
};

std::ostream& operator<<(std::ostream& os, const Token& t);
