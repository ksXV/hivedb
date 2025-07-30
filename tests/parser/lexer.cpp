#include <string_view>

#include <catch_amalgamated.hpp>

#include <parser/lexer.hpp>
#include <parser/tokens.hpp>

TEST_CASE("Lexer tests for symbols", "[lexer]") {
    using namespace hivedb;
    constexpr std::string_view input = "(1+1);";
    lexer l{input};

    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 6> results {
        token{token_type::parenthesesL},
        token{token_type::integer, "1"},
        token{token_type::add},
        token{token_type::integer, "1"},
        token{token_type::parenthesesR},
        token{token_type::eof},
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
    lexer l{input};
    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 5> results {
        token{token_type::select, "select"},
        token{token_type::parenthesesL},
        token{token_type::add},
        token{token_type::parenthesesR},
        token{token_type::eof}
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
    lexer l{input};
    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 5> results {
        token{token_type::select, "select"},
        token{token_type::string, "foo"},
        token{token_type::from, "from"},
        token{token_type::string, "bar"},
        token{token_type::eof}
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
    lexer l{input};

    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 5> results {
        token{token_type::select, "select"},
        token{token_type::identifier, "sum"},
        token{token_type::parenthesesL},
        token{token_type::parenthesesR},
        token{token_type::eof},
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
    lexer l{input};

    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 6> results {
        token{token_type::string, "foo"},
        token{token_type::comma},
        token{token_type::string, "bar"},
        token{token_type::comma},
        token{token_type::string, "goo"},
        token{token_type::eof}
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
    lexer l{input};

    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 8> results {
        token{token_type::parenthesesL},
        token{token_type::string, "foo"},
        token{token_type::comma},
        token{token_type::string, "bar"},
        token{token_type::comma},
        token{token_type::string, "goo"},
        token{token_type::parenthesesR},
        token{token_type::eof}
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
    lexer l{input};

    std::vector<token> lexerResults = l.getTokens();

    const std::array<token, 4> results {
        token{token_type::identifier, "foo"},
        token{token_type::dot},
        token{token_type::identifier, "bar"},
        token{token_type::eof},
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
