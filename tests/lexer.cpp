#include <string_view>
#include <array>

#include <catch_amalgamated.hpp>

#include "parser/lexer.h"
#include "parser/tokens.h"

TEST_CASE("Lexer tests for symbols", "[lexer]") {
    constexpr std::string_view input = "()\"";
    Lexer l{input};

    const std::array<Token, 3> results {
        Token{TokenType::parenthesesL, '('},
        {TokenType::parenthesesR, ')'},
        {TokenType::quoationMark, '"'}
    };
    for (const auto& curentToken : results) {
        auto nextToken = l.nextToken();
        REQUIRE(nextToken == curentToken);
    }
}

TEST_CASE("Lexer tests for simple select", "[lexer]") {
    constexpr std::string_view input = "select ( + )";
    Lexer l{input};

    const std::array<Token, 4> results {
        Token{TokenType::select, "select"},
        {TokenType::parenthesesL, '('},
        {TokenType::plusOperator, '+'},
        {TokenType::parenthesesR, ')'}
    };
    for (const auto& curentToken : results) {
        auto nextToken = l.nextToken();
        REQUIRE(nextToken == curentToken);
    }
}

TEST_CASE("Lexer tests for select", "[lexer]") {
    constexpr std::string_view input = "select foo from bar";
    Lexer l{input};

    const std::array<Token, 4> results {
        Token{TokenType::select, "select"},
        {TokenType::identifier, "foo"},
        {TokenType::from, "from"},
        {TokenType::identifier, "bar"}
    };
    for (const auto& curentToken : results) {
        auto nextToken = l.nextToken();
        REQUIRE(nextToken == curentToken);
    }
}
