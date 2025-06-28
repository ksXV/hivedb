#include <string_view>

#include <catch_amalgamated.hpp>

#include <parser/lexer.hpp>
#include <parser/tokens.hpp>

TEST_CASE("Lexer tests for symbols", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "(1+1);";
    Lexer l{input};

    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 6> results {
        Token{TokenType::parenthesesL},
        Token{TokenType::integer, "1"},
        Token{TokenType::add},
        Token{TokenType::integer, "1"},
        Token{TokenType::parenthesesR},
        Token{TokenType::eof},
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer tests for simple select", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "select ( + );";
    Lexer l{input};
    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 5> results {
        Token{TokenType::select, "select"},
        Token{TokenType::parenthesesL},
        Token{TokenType::add},
        Token{TokenType::parenthesesR},
        Token{TokenType::eof}
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer tests for select", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "select \"foo\" from \"bar\";";
    Lexer l{input};
    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 9> results {
        Token{TokenType::select, "select"},
        Token{TokenType::quote},
        Token{TokenType::identifier, "foo"},
        Token{TokenType::quote},
        Token{TokenType::from, "from"},
        Token{TokenType::quote},
        Token{TokenType::identifier, "bar"},
        Token{TokenType::quote},
        Token{TokenType::eof}
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer test for functions", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "select sum();";
    Lexer l{input};

    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 5> results {
        Token{TokenType::select, "select"},
        Token{TokenType::identifier, "sum"},
        Token{TokenType::parenthesesL},
        Token{TokenType::parenthesesR},
        Token{TokenType::eof},
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer identifier", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "\"foo\",\"bar\",\"goo\";";
    Lexer l{input};

    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 12> results {
        Token{TokenType::quote},
        Token{TokenType::identifier, "foo"},
        Token{TokenType::quote},
        Token{TokenType::comma},
        Token{TokenType::quote},
        Token{TokenType::identifier, "bar"},
        Token{TokenType::quote},
        Token{TokenType::comma},
        Token{TokenType::quote},
        Token{TokenType::identifier, "goo"},
        Token{TokenType::quote},
        Token{TokenType::eof}
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer identifier with parantheses", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "(\"foo\",\"bar\",\"goo\");";
    Lexer l{input};

    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 14> results {
        Token{TokenType::parenthesesL},
        Token{TokenType::quote},
        Token{TokenType::identifier, "foo"},
        Token{TokenType::quote},
        Token{TokenType::comma},
        Token{TokenType::quote},
        Token{TokenType::identifier, "bar"},
        Token{TokenType::quote},
        Token{TokenType::comma},
        Token{TokenType::quote},
        Token{TokenType::identifier, "goo"},
        Token{TokenType::quote},
        Token{TokenType::parenthesesR},
        Token{TokenType::eof}
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        REQUIRE(nextToken == nextTokenLexer);
    }
}

TEST_CASE("Lexer identifier for database.table", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "foo.bar;";
    Lexer l{input};

    std::vector<Token> lexerResults = l.getTokens();

    const std::array<Token, 4> results {
        Token{TokenType::identifier, "foo"},
        Token{TokenType::dot},
        Token{TokenType::identifier, "bar"},
        Token{TokenType::eof},
    };

    REQUIRE(!lexerResults.empty());
    REQUIRE(lexerResults.size() == results.size());

    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        const auto& nextTokenLexer = lexerResults[idx];
        const auto& nextToken = results[idx];
        fmt::println("{}", nextTokenLexer);
        REQUIRE(nextToken == nextTokenLexer);
    }
}
