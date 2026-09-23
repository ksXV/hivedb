#include <exception>
#include <filesystem>
#include <string_view>
#include <variant>
#include <vector>

#include <catch_amalgamated.hpp>

#include <parser/lexer.hpp>
#include <parser/parser.hpp>
#include <storage_engine/storage_engine.hpp>

TEST_CASE("Storage Engine - Strategy A: Multi-table B+ tree in a single file", "[storage_engine]") {
    hivedb::storage_engine engine{};

    SECTION("Create multiple tables and duplicate detection") {
        // Table 1: users
        {
            constexpr std::string_view sql = "create table users (id int, name varchar, age int);";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto* createExpr = dynamic_cast<hivedb::create_tbl_expr*>(ast.get());
            REQUIRE(createExpr != nullptr);
            REQUIRE_NOTHROW(engine.createTable(createExpr));
        }

        // Duplicate table fails
        {
            constexpr std::string_view sql = "create table users (id int);";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto* createExpr = dynamic_cast<hivedb::create_tbl_expr*>(ast.get());
            REQUIRE(createExpr != nullptr);
            REQUIRE_THROWS_AS(engine.createTable(createExpr), std::runtime_error);
        }

        // Table 2: orders in the exact same database file
        {
            constexpr std::string_view sql = "create table orders (order_id int, user_id int, amount real);";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto* createExpr = dynamic_cast<hivedb::create_tbl_expr*>(ast.get());
            REQUIRE(createExpr != nullptr);
            REQUIRE_NOTHROW(engine.createTable(createExpr));
        }
    }

    SECTION("Insert and Query across multiple tables with WHERE and Wildcard") {
        // Create table 1: users
        {
            constexpr std::string_view sql = "create table users (id int, name varchar, age int);";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            engine.createTable(dynamic_cast<hivedb::create_tbl_expr*>(ast.get()));
        }

        // Create table 2: orders
        {
            constexpr std::string_view sql = "create table orders (order_id int, user_id int, amount real);";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            engine.createTable(dynamic_cast<hivedb::create_tbl_expr*>(ast.get()));
        }

        // Insert rows into users
        {
            constexpr std::string_view sql1 = "insert into users (id, name, age) values (1, \"Alice\", 30);";
            hivedb::lexer l1{sql1};
            auto ast1 = hivedb::parser{l1.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast1.get()));

            constexpr std::string_view sql2 = "insert into users (id, name, age) values (2, \"Bob\", 20);";
            hivedb::lexer l2{sql2};
            auto ast2 = hivedb::parser{l2.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast2.get()));

            constexpr std::string_view sql3 = "insert into users (id, name, age) values (3, \"Charlie\", 45);";
            hivedb::lexer l3{sql3};
            auto ast3 = hivedb::parser{l3.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast3.get()));
        }

        // Insert rows into orders
        {
            constexpr std::string_view sql1 = "insert into orders (order_id, user_id, amount) values (101, 1, 99.5);";
            hivedb::lexer l1{sql1};
            auto ast1 = hivedb::parser{l1.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast1.get()));

            constexpr std::string_view sql2 = "insert into orders (order_id, user_id, amount) values (102, 2, 15.0);";
            hivedb::lexer l2{sql2};
            auto ast2 = hivedb::parser{l2.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast2.get()));

            constexpr std::string_view sql3 = "insert into orders (order_id, user_id, amount) values (103, 1, 250.0);";
            hivedb::lexer l3{sql3};
            auto ast3 = hivedb::parser{l3.getTokens()}.parse();
            engine.insertIntoTable(dynamic_cast<hivedb::insert_expr*>(ast3.get()));
        }

        // Test Wildcard SELECT * FROM users
        {
            constexpr std::string_view sql = "select * from users;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.column_names.size() == 3);
            REQUIRE(res.column_names[0] == "id");
            REQUIRE(res.column_names[1] == "name");
            REQUIRE(res.column_names[2] == "age");
            REQUIRE(res.rows.size() == 3);

            REQUIRE(std::get<int>(res.rows[0][0]) == 1);
            REQUIRE(std::get<std::string_view>(res.rows[0][1]) == "Alice");
            REQUIRE(std::get<int>(res.rows[0][2]) == 30);

            REQUIRE(std::get<int>(res.rows[1][0]) == 2);
            REQUIRE(std::get<std::string_view>(res.rows[1][1]) == "Bob");
            REQUIRE(std::get<int>(res.rows[1][2]) == 20);

            REQUIRE(std::get<int>(res.rows[2][0]) == 3);
            REQUIRE(std::get<std::string_view>(res.rows[2][1]) == "Charlie");
            REQUIRE(std::get<int>(res.rows[2][2]) == 45);
        }

        // Test Wildcard SELECT * FROM orders
        {
            constexpr std::string_view sql = "select * from orders;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.column_names.size() == 3);
            REQUIRE(res.column_names[0] == "order_id");
            REQUIRE(res.column_names[1] == "user_id");
            REQUIRE(res.column_names[2] == "amount");
            REQUIRE(res.rows.size() == 3);

            REQUIRE(std::get<int>(res.rows[0][0]) == 101);
            REQUIRE(std::get<int>(res.rows[0][1]) == 1);
            REQUIRE(std::get<float>(res.rows[0][2]) == Catch::Approx(99.5f));

            REQUIRE(std::get<int>(res.rows[1][0]) == 102);
            REQUIRE(std::get<int>(res.rows[1][1]) == 2);
            REQUIRE(std::get<float>(res.rows[1][2]) == Catch::Approx(15.0f));

            REQUIRE(std::get<int>(res.rows[2][0]) == 103);
            REQUIRE(std::get<int>(res.rows[2][1]) == 1);
            REQUIRE(std::get<float>(res.rows[2][2]) == Catch::Approx(250.0f));
        }

        // Test Column Projections: SELECT name, age FROM users;
        {
            constexpr std::string_view sql = "select name, age from users;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.column_names.size() == 2);
            REQUIRE(res.column_names[0] == "name");
            REQUIRE(res.column_names[1] == "age");
            REQUIRE(res.rows.size() == 3);
            REQUIRE(std::get<std::string_view>(res.rows[0][0]) == "Alice");
            REQUIRE(std::get<int>(res.rows[0][1]) == 30);
        }

        // Test WHERE clause with comparison: SELECT name, age FROM users WHERE age >= 25;
        {
            constexpr std::string_view sql = "select name, age from users where age >= 25;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.rows.size() == 2);
            REQUIRE(std::get<std::string_view>(res.rows[0][0]) == "Alice");
            REQUIRE(std::get<int>(res.rows[0][1]) == 30);
            REQUIRE(std::get<std::string_view>(res.rows[1][0]) == "Charlie");
            REQUIRE(std::get<int>(res.rows[1][1]) == 45);
        }

        // Test WHERE equality on string: SELECT * FROM users WHERE name == "Bob";
        {
            constexpr std::string_view sql = "select * from users where name == \"Bob\";";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.rows.size() == 1);
            REQUIRE(std::get<int>(res.rows[0][0]) == 2);
            REQUIRE(std::get<std::string_view>(res.rows[0][1]) == "Bob");
            REQUIRE(std::get<int>(res.rows[0][2]) == 20);
        }

        // Test WHERE filter on real: SELECT order_id, amount FROM orders WHERE amount > 50.0;
        {
            constexpr std::string_view sql = "select order_id, amount from orders where amount > 50.0;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.rows.size() == 2);
            REQUIRE(std::get<int>(res.rows[0][0]) == 101);
            REQUIRE(std::get<int>(res.rows[1][0]) == 103);
        }

        // Test WHERE filter with no matches: SELECT * FROM users WHERE age < 10;
        {
            constexpr std::string_view sql = "select * from users where age < 10;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
            REQUIRE(res.rows.empty());
        }

        // Test error handling: query non-existent table throws
        {
            constexpr std::string_view sql = "select * from nonexistent;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            REQUIRE_THROWS_AS(engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get())), std::runtime_error);
        }

        // Test error handling: query invalid column throws
        {
            constexpr std::string_view sql = "select nonexistent_column from users;";
            hivedb::lexer l{sql};
            auto ast = hivedb::parser{l.getTokens()}.parse();
            REQUIRE_THROWS_AS(engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get())), std::runtime_error);
        }
    }

    SECTION("Scalar queries execution") {
        constexpr std::string_view sql = "select ( 10 + 20 );";
        hivedb::lexer l{sql};
        auto ast = hivedb::parser{l.getTokens()}.parse();
        auto res = engine.executeSelect(dynamic_cast<hivedb::select_expr*>(ast.get()));
        REQUIRE(res.rows.size() == 1);
        REQUIRE(std::get<int>(res.rows[0][0]) == 30);
    }
}
