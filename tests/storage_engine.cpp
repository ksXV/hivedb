#include "parser/parser.hpp"
#include <catch_amalgamated.hpp>
#include <exception>
#include <iostream>
#include <storage_engine/storage_engine.hpp>



TEST_CASE("Storage Engine", "[basic storage engine test: create a table]") {
    using namespace hivedb;

    try {
        constexpr std::string_view input = "create table foo (bam integer, vam varchar, zam real);";
        lexer l{input};
        parser ast{l.getTokens()};
        auto root = ast.parse();
        auto createTblExpr = static_cast<create_tbl_expr*>(root.get());

        storage_engine engine{};
        engine.createTable(createTblExpr);
        REQUIRE(1 == 1);
    } catch(std::exception& err) {
       FAIL(err.what());
    }
}


TEST_CASE("Storage Engine", "[basic inserting]") {
    using namespace hivedb;

    try {
        constexpr std::string_view createStmt = "create table vbam (vram integer, bam integer);";
        lexer l1{createStmt};
        parser p1{l1.getTokens()};
        auto root = p1.parse();
        auto createTblExpr = static_cast<create_tbl_expr*>(root.get());

        storage_engine engine{};
        engine.createTable(createTblExpr);

        constexpr std::string_view insertStmt = "insert into vbam (bam) values (1);";
        lexer l2{insertStmt};
        parser p2{l2.getTokens()};
        auto root2 = p2.parse();
        auto insertExpr = static_cast<insert_expr*>(root2.get());

        engine.insertIntoTable(insertExpr);
        REQUIRE(1 == 1);
    } catch(std::exception& err) {
       FAIL(err.what());
    }
}

TEST_CASE("Storage Engine", "[insert varchar]") {
    using namespace hivedb;

    try {
        constexpr std::string_view createStmt = "create table vbam (vram integer, bam varchar);";
        lexer l1{createStmt};
        parser p1{l1.getTokens()};
        auto root = p1.parse();
        auto createTblExpr = static_cast<create_tbl_expr*>(root.get());

        storage_engine engine{};
        engine.createTable(createTblExpr);

        constexpr std::string_view insertStmt = "insert into vbam (vram, bam) values (100, \"ilovegaysexso\");";
        lexer l2{insertStmt};
        parser p2{l2.getTokens()};
        auto root2 = p2.parse();
        auto insertExpr = static_cast<insert_expr*>(root2.get());

        engine.insertIntoTable(insertExpr);
        REQUIRE(1 == 1);
    } catch(std::exception& err) {
       FAIL(err.what());
    }
}

TEST_CASE("Storage Engine", "[select without columns]") {
    using namespace hivedb;

    try {
        constexpr std::string_view selectStmt = "select (\"i love bald joe\", (select 1));";
        lexer l1{selectStmt};
        parser p1{l1.getTokens()};
        auto root = p1.parse();
        auto selectTblExpr = static_cast<select_expr*>(root.get());
        std::stringstream ss;
        selectTblExpr->prettyPrint(ss);
        std::cout << "\n" << ss.str();

        storage_engine engine{};
        engine.queryDataFromTable(selectTblExpr);

        REQUIRE(1 == 1);
    } catch(std::exception& err) {
       FAIL(err.what());
    }
}

TEST_CASE("Storage Engine", "[select with columns]") {
    using namespace hivedb;
    try {
        constexpr std::string_view createStmt = "create table vbam (vram integer, bam varchar);";
        lexer l1{createStmt};
        parser p1{l1.getTokens()};
        auto root = p1.parse();
        auto createTblExpr = static_cast<create_tbl_expr*>(root.get());

        storage_engine engine{};
        engine.createTable(createTblExpr);

        constexpr std::string_view insertStmt = "insert into vbam (vram, bam) values (100, \"ilove\");";
        lexer l2{insertStmt};
        parser p2{l2.getTokens()};
        auto root2 = p2.parse();
        auto insertExpr = static_cast<insert_expr*>(root2.get());

        engine.insertIntoTable(insertExpr);

        constexpr std::string_view insertStmt2 = "insert into vbam (vram, bam) values (102, \"ilovegaysex\");";
        lexer l4{insertStmt2};
        parser p4{l4.getTokens()};
        auto root4 = p4.parse();
        auto insertExpr2 = static_cast<insert_expr*>(root4.get());

        engine.insertIntoTable(insertExpr2);

        constexpr std::string_view selectStmt = "select (bam, vram+2) from vbam;";
        lexer l3{selectStmt};
        parser p3{l3.getTokens()};
        auto root3 = p3.parse();
        auto selectExpr = static_cast<select_expr*>(root3.get());
        engine.queryDataFromTable(selectExpr);
        REQUIRE(1 == 1);
    } catch(std::exception& err) {
       FAIL(err.what());
    }
}
