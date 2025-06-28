#pragma once

#include <string_view>

#include <fmt/core.h>
#include <fmt/format.h>

namespace hivedb {
enum class TokenType {
    select,
    from,

    create,
    table,
    _not,
    null,

    insert,
    into,
    values,

    parenthesesL,
    parenthesesR,

    add,
    substract,
    star,
    divide,
    bang,

    quote,
    comma,
    dot,

    identifier,
    integer,
    real,

    eof,

    illegal,
};


struct Token {
    TokenType type{};
    std::string_view literal{};

    Token() = default;
    explicit Token(TokenType t);
    Token(TokenType t, std::string_view l);


    friend bool operator==(const Token&, const Token&) noexcept;

    friend std::ostream& operator<<(std::ostream& os, const Token& t);

    [[nodiscard]] std::string name() const;
};
}

// for fmt::format
template <>
struct fmt::formatter<hivedb::Token> : formatter<std::string> {
    auto format(hivedb::Token t, format_context& ctx) const {
        return formatter<std::string>::format(t.name(), ctx);
    }
};
