#include <string_view>

#include <catch_amalgamated.hpp>

#include <parser/lexer.hpp>
#include <parser/tokens.hpp>

TEST_CASE("Lexer tests for symbols", "[lexer]") {
    constexpr std::string_view input = "(1+1);";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 6> results {
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::integer, "1"},
        hivedb::token{hivedb::token_type::add},
        hivedb::token{hivedb::token_type::integer, "1"},
        hivedb::token{hivedb::token_type::parenthesesR},
        hivedb::token{hivedb::token_type::eof},
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
    constexpr std::string_view input = "select ( + );";
    hivedb::lexer l{input};
    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 5> results {
        hivedb::token{hivedb::token_type::select, "select"},
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::add},
        hivedb::token{hivedb::token_type::parenthesesR},
        hivedb::token{hivedb::token_type::eof}
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
    constexpr std::string_view input = "select \"foo\" from \"bar\";";
    hivedb::lexer l{input};
    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 5> results {
        hivedb::token{hivedb::token_type::select, "select"},
        hivedb::token{hivedb::token_type::string, "foo"},
        hivedb::token{hivedb::token_type::from, "from"},
        hivedb::token{hivedb::token_type::string, "bar"},
        hivedb::token{hivedb::token_type::eof}
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
    constexpr std::string_view input = "select sum();";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 5> results {
        hivedb::token{hivedb::token_type::select, "select"},
        hivedb::token{hivedb::token_type::identifier, "sum"},
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::parenthesesR},
        hivedb::token{hivedb::token_type::eof},
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
    constexpr std::string_view input = "\"foo\",\"bar\",\"goo\";";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 6> results {
        hivedb::token{hivedb::token_type::string, "foo"},
        hivedb::token{hivedb::token_type::comma},
        hivedb::token{hivedb::token_type::string, "bar"},
        hivedb::token{hivedb::token_type::comma},
        hivedb::token{hivedb::token_type::string, "goo"},
        hivedb::token{hivedb::token_type::eof}
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
    constexpr std::string_view input = "(\"foo\",\"bar\",\"goo\");";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 8> results {
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::string, "foo"},
        hivedb::token{hivedb::token_type::comma},
        hivedb::token{hivedb::token_type::string, "bar"},
        hivedb::token{hivedb::token_type::comma},
        hivedb::token{hivedb::token_type::string, "goo"},
        hivedb::token{hivedb::token_type::parenthesesR},
        hivedb::token{hivedb::token_type::eof}
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
    constexpr std::string_view input = "foo.bar;";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 4> results {
        hivedb::token{hivedb::token_type::identifier, "foo"},
        hivedb::token{hivedb::token_type::dot},
        hivedb::token{hivedb::token_type::identifier, "bar"},
        hivedb::token{hivedb::token_type::eof},
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

TEST_CASE("Lexer floating point numbers", "[lexer]") {
    constexpr std::string_view input = "3.14 0.007 123.456;";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 4> results {
        hivedb::token{hivedb::token_type::real, "3.14"},
        hivedb::token{hivedb::token_type::real, "0.007"},
        hivedb::token{hivedb::token_type::real, "123.456"},
        hivedb::token{hivedb::token_type::eof},
    };

    REQUIRE(lexerResults.size() == results.size());
    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        REQUIRE(results[idx] == lexerResults[idx]);
    }
}

TEST_CASE("Lexer create table keywords", "[lexer]") {
    constexpr std::string_view input = "create table t (id int not null);";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 9> results {
        hivedb::token{hivedb::token_type::create, "create"},
        hivedb::token{hivedb::token_type::table, "table"},
        hivedb::token{hivedb::token_type::identifier, "t"},
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::identifier, "id"},
        hivedb::token{hivedb::token_type::identifier, "int"},
        hivedb::token{hivedb::token_type::_not, "not"},
        hivedb::token{hivedb::token_type::null, "null"},
        hivedb::token{hivedb::token_type::parenthesesR},
    };

    REQUIRE(lexerResults.size() == results.size() + 1); // + eof
    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        REQUIRE(results[idx] == lexerResults[idx]);
    }
    REQUIRE(lexerResults.back() == hivedb::token{hivedb::token_type::eof});
}

TEST_CASE("Lexer insert statement keywords", "[lexer]") {
    constexpr std::string_view input = "insert into users (name) values (\"alice\");";
    hivedb::lexer l{input};

    std::vector<hivedb::token> lexerResults = l.getTokens();

    const std::array<hivedb::token, 9> results {
        hivedb::token{hivedb::token_type::insert, "insert"},
        hivedb::token{hivedb::token_type::into, "into"},
        hivedb::token{hivedb::token_type::identifier, "users"},
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::identifier, "name"},
        hivedb::token{hivedb::token_type::parenthesesR},
        hivedb::token{hivedb::token_type::values, "values"},
        hivedb::token{hivedb::token_type::parenthesesL},
        hivedb::token{hivedb::token_type::string, "alice"},
    };

    REQUIRE(lexerResults.size() == results.size() + 2); // + ) and eof
    for (std::size_t idx = 0; idx < results.size(); ++idx) {
        REQUIRE(results[idx] == lexerResults[idx]);
    }
    REQUIRE(lexerResults[lexerResults.size() - 2] == hivedb::token{hivedb::token_type::parenthesesR});
    REQUIRE(lexerResults.back() == hivedb::token{hivedb::token_type::eof});
}

TEST_CASE("Lexer comparison operators and WHERE/AND/OR keywords", "[lexer]") {
    SECTION("Comparison operators") {
        constexpr std::string_view input = "= == != <> < <= > >=;";
        hivedb::lexer l{input};
        std::vector<hivedb::token> tokens = l.getTokens();

        const std::array expected {
            hivedb::token_type::equal,
            hivedb::token_type::equal,
            hivedb::token_type::not_equal,
            hivedb::token_type::not_equal,
            hivedb::token_type::less,
            hivedb::token_type::less_equal,
            hivedb::token_type::greater,
            hivedb::token_type::greater_equal,
            hivedb::token_type::eof,
        };

        REQUIRE(tokens.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            REQUIRE(tokens[i].type == expected[i]);
        }
    }

    SECTION("Case-insensitive keywords: where, and, or") {
        constexpr std::string_view input = "where WHERE and AND or OR;";
        hivedb::lexer l{input};
        std::vector<hivedb::token> tokens = l.getTokens();

        const std::array expected {
            hivedb::token_type::where,
            hivedb::token_type::where,
            hivedb::token_type::_and,
            hivedb::token_type::_and,
            hivedb::token_type::_or,
            hivedb::token_type::_or,
            hivedb::token_type::eof,
        };

        REQUIRE(tokens.size() == expected.size());
        for (std::size_t i = 0; i < expected.size(); ++i) {
            REQUIRE(tokens[i].type == expected[i]);
        }
    }
}

TEST_CASE("Lexer wildcard star in projection", "[lexer]") {
    constexpr std::string_view input = "select * from tbl;";
    hivedb::lexer l{input};
    std::vector<hivedb::token> tokens = l.getTokens();

    const std::array expected {
        hivedb::token_type::select,
        hivedb::token_type::star,
        hivedb::token_type::from,
        hivedb::token_type::identifier,
        hivedb::token_type::eof,
    };

    REQUIRE(tokens.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        REQUIRE(tokens[i].type == expected[i]);
    }
}
