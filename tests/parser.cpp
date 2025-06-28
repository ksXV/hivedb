#include <fmt/core.h>
#include <exception>
#include <sstream>

#include <catch_amalgamated.hpp>

#include <parser/parser.hpp>

TEST_CASE("Parser", "[Simple select]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "select ( 1 + 1 );";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[Double simple select]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "select(select(69+420));";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[No select]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "(69+420);";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        FAIL("Should not reach here!");
    } catch (const std::exception& error) {
        if (std::string_view(error.what()) == "Invalid token: parenthesesL") {
            REQUIRE(1 == 1);
        } else {
            FAIL(error.what());
        }
    }
}

TEST_CASE("Parser", "[Simple select with identifier]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "select \"foobar\";";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[Simple select with multiple identifiers]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "select (\"foobar\", \"foo\");";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[More complex select with multiple identifiers]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "select (\"foobar\", \"foo\", \"foobarfoo\") from baz;";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }

    try {
        constexpr std::string_view input = "select (\"foobar\", \"foo\", \"foobarfoo\") from foo.baz;";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[create table test]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "create table foo (bar varchar, baffle int, bastard float not null);";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}

TEST_CASE("Parser", "[insert table test]") {
    using namespace hivedb;
    try {
        constexpr std::string_view input = "insert into foo (bar, zar) values (bam, vam);";
        Lexer l{input};
        Parser ast{l.getTokens()};
        const auto root = ast.parse();
        std::stringstream stmt;
        root->prettyPrint(stmt);
        std::string stmt_s = stmt.str();
        fmt::println("{}", stmt_s);
        REQUIRE(1 == 1);
    } catch (const std::exception& error) {
        FAIL(error.what());
    }
}
