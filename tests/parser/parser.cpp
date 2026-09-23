#include <exception>
#include <filesystem>
#include <sstream>
#include <variant>
#include <vector>

#include <catch_amalgamated.hpp>
#include <fmt/core.h>

#include <parser/lexer.hpp>
#include <parser/parser.hpp>

TEST_CASE("Parser - Simple select arithmetic", "[parser]") {
    constexpr std::string_view input = "select ( 1 + 1 );";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
    REQUIRE(sel != nullptr);
    REQUIRE(sel->tblName.empty());
    REQUIRE(sel->innerExpr.size() == 1);

    const auto* bin = dynamic_cast<const hivedb::binary_expr*>(sel->innerExpr[0].get());
    REQUIRE(bin != nullptr);
    REQUIRE(bin->op == hivedb::token_type::add);

    const auto* lhs = dynamic_cast<const hivedb::literal_expr<int>*>(bin->lhs.get());
    REQUIRE(lhs != nullptr);
    REQUIRE(lhs->value == 1);
    REQUIRE_FALSE(lhs->isIdentifier);

    const auto* rhs = dynamic_cast<const hivedb::literal_expr<int>*>(bin->rhs.get());
    REQUIRE(rhs != nullptr);
    REQUIRE(rhs->value == 1);
    REQUIRE_FALSE(rhs->isIdentifier);

    const auto result = root->execute();
    REQUIRE(std::holds_alternative<int>(result));
    REQUIRE(std::get<int>(result) == 2);
}

TEST_CASE("Parser - Nested subqueries evaluation", "[parser]") {
    constexpr std::string_view input = "select(select(select((select(select(69+420))))))";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
    REQUIRE(sel != nullptr);
    REQUIRE(sel->innerExpr.size() == 1);

    const auto result = root->execute();
    REQUIRE(std::holds_alternative<int>(result));
    REQUIRE(std::get<int>(result) == 489);
}

TEST_CASE("Parser - Non-statement expression fails", "[parser]") {
    constexpr std::string_view input = "(69+420);";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
}

TEST_CASE("Parser - Select string literal vs column identifier", "[parser]") {
    SECTION("String literal in projection") {
        constexpr std::string_view input = "select (\"foobar\");";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName.empty());
        REQUIRE(sel->innerExpr.size() == 1);

        const auto* lit = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[0].get());
        REQUIRE(lit != nullptr);
        REQUIRE(lit->value == "foobar");
        REQUIRE_FALSE(lit->isIdentifier);
    }

    SECTION("Column identifier in projection") {
        constexpr std::string_view input = "select (foobar);";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName.empty());
        REQUIRE(sel->innerExpr.size() == 1);

        const auto* lit = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[0].get());
        REQUIRE(lit != nullptr);
        REQUIRE(lit->value == "foobar");
        REQUIRE(lit->isIdentifier);
    }
}

TEST_CASE("Parser - Select multiple projections", "[parser]") {
    constexpr std::string_view input = "select (\"foobar\", \"foo\");";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
    REQUIRE(sel != nullptr);
    REQUIRE(sel->innerExpr.size() == 2);

    const auto* lit1 = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[0].get());
    REQUIRE(lit1 != nullptr);
    REQUIRE(lit1->value == "foobar");

    const auto* lit2 = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[1].get());
    REQUIRE(lit2 != nullptr);
    REQUIRE(lit2->value == "foo");
}

TEST_CASE("Parser - Select multiple projections from table", "[parser]") {
    constexpr std::string_view input = "select (\"foobar\", \"foo\", \"foobarfoo\") from baz;";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
    REQUIRE(sel != nullptr);
    REQUIRE(sel->tblName == "baz");
    REQUIRE(sel->innerExpr.size() == 3);

    const auto* lit1 = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[0].get());
    REQUIRE(lit1 != nullptr);
    REQUIRE(lit1->value == "foobar");

    const auto* lit2 = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[1].get());
    REQUIRE(lit2 != nullptr);
    REQUIRE(lit2->value == "foo");

    const auto* lit3 = dynamic_cast<const hivedb::literal_expr<std::string>*>(sel->innerExpr[2].get());
    REQUIRE(lit3 != nullptr);
    REQUIRE(lit3->value == "foobarfoo");
}

TEST_CASE("Parser - Create table statement", "[parser]") {
    constexpr std::string_view input = "create table foo (bar varchar, baffle int, bastard float not null);";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* create_tbl = dynamic_cast<const hivedb::create_tbl_expr*>(root.get());
    REQUIRE(create_tbl != nullptr);
    REQUIRE(create_tbl->tblName == "foo");
    REQUIRE(create_tbl->tblColumns.size() == 3);

    REQUIRE(create_tbl->tblColumns[0].name == "bar");
    REQUIRE(create_tbl->tblColumns[0].type == "varchar");
    REQUIRE(create_tbl->tblColumns[0].can_be_null == true);

    REQUIRE(create_tbl->tblColumns[1].name == "baffle");
    REQUIRE(create_tbl->tblColumns[1].type == "int");
    REQUIRE(create_tbl->tblColumns[1].can_be_null == true);

    REQUIRE(create_tbl->tblColumns[2].name == "bastard");
    REQUIRE(create_tbl->tblColumns[2].type == "float");
    REQUIRE(create_tbl->tblColumns[2].can_be_null == false);
}

TEST_CASE("Parser - Insert statement with string values", "[parser]") {
    constexpr std::string_view input = "insert into foo (bar, zar) values (\"bam\", \"vam\");";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* ins = dynamic_cast<const hivedb::insert_expr*>(root.get());
    REQUIRE(ins != nullptr);
    REQUIRE(ins->tblName == "foo");

    REQUIRE(ins->columns.size() == 2);
    REQUIRE(ins->columns[0] == "bar");
    REQUIRE(ins->columns[1] == "zar");

    REQUIRE(ins->values.size() == 2);
    REQUIRE(std::holds_alternative<std::string_view>(ins->values[0]));
    REQUIRE(std::get<std::string_view>(ins->values[0]) == "bam");
    REQUIRE(std::holds_alternative<std::string_view>(ins->values[1]));
    REQUIRE(std::get<std::string_view>(ins->values[1]) == "vam");
}

TEST_CASE("Parser - Insert statement with mixed literal types", "[parser]") {
    constexpr std::string_view input = "insert into items (id, name) values (42, \"apple\");";
    hivedb::lexer l{input};
    hivedb::parser ast{l.getTokens()};
    const auto root = ast.parse();

    REQUIRE(root != nullptr);
    const auto* ins = dynamic_cast<const hivedb::insert_expr*>(root.get());
    REQUIRE(ins != nullptr);
    REQUIRE(ins->tblName == "items");

    REQUIRE(ins->columns.size() == 2);
    REQUIRE(ins->columns[0] == "id");
    REQUIRE(ins->columns[1] == "name");

    REQUIRE(ins->values.size() == 2);
    REQUIRE(std::holds_alternative<int>(ins->values[0]));
    REQUIRE(std::get<int>(ins->values[0]) == 42);
    REQUIRE(std::holds_alternative<std::string_view>(ins->values[1]));
    REQUIRE(std::get<std::string_view>(ins->values[1]) == "apple");
}

TEST_CASE("Parser - Arithmetic execution evaluation", "[parser]") {
    SECTION("Subtraction") {
        constexpr std::string_view input = "select ( 20 - 7 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 13);
    }

    SECTION("Multiplication") {
        constexpr std::string_view input = "select ( 6 * 7 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 42);
    }

    SECTION("Division") {
        constexpr std::string_view input = "select ( 40 / 5 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 8);
    }

    SECTION("Unary minus") {
        constexpr std::string_view input = "select ( -42 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == -42);
    }
}

TEST_CASE("Parser - Wildcard projection", "[parser]") {
    SECTION("SELECT * FROM table") {
        constexpr std::string_view input = "select * from users;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName == "users");
        REQUIRE(sel->hasWildcard());
        REQUIRE(sel->innerExpr.size() == 1);
        REQUIRE(dynamic_cast<const hivedb::wildcard_expr*>(sel->innerExpr[0].get()) != nullptr);

        const auto cols = sel->retriveColumnsToBeFetched();
        REQUIRE(cols.size() == 1);
        REQUIRE(cols[0] == "*");
    }

    SECTION("SELECT (*) FROM table with parentheses") {
        constexpr std::string_view input = "select (*) from users;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->hasWildcard());
        REQUIRE(sel->innerExpr.size() == 1);
        REQUIRE(dynamic_cast<const hivedb::wildcard_expr*>(sel->innerExpr[0].get()) != nullptr);
    }

    SECTION("SELECT *, id FROM table") {
        constexpr std::string_view input = "select *, id from users;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->hasWildcard());
        REQUIRE(sel->innerExpr.size() == 2);
        REQUIRE(dynamic_cast<const hivedb::wildcard_expr*>(sel->innerExpr[0].get()) != nullptr);

        const auto cols = sel->retriveColumnsToBeFetched();
        REQUIRE(cols.size() == 2);
        REQUIRE(cols[0] == "*");
        REQUIRE(cols[1] == "id");
    }
}

TEST_CASE("Parser - Operator precedence and associativity", "[parser]") {
    SECTION("Multiplicative has higher precedence than additive: 2 + 3 * 4 == 14") {
        constexpr std::string_view input = "select ( 2 + 3 * 4 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 14);
    }

    SECTION("Multiplicative has higher precedence: 20 - 2 * 5 == 10") {
        constexpr std::string_view input = "select ( 20 - 2 * 5 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 10);
    }

    SECTION("Left associativity of subtraction: 20 - 5 - 3 == 12") {
        constexpr std::string_view input = "select ( 20 - 5 - 3 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 12);
    }

    SECTION("Left associativity of division: 24 / 4 / 2 == 3") {
        constexpr std::string_view input = "select ( 24 / 4 / 2 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 3);
    }

    SECTION("Parentheses override precedence: (2 + 3) * 4 == 20") {
        constexpr std::string_view input = "select ( (2 + 3) * 4 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == 20);
    }

    SECTION("Unary operators: -3 * 4 == -12") {
        constexpr std::string_view input = "select ( -3 * 4 );";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();
        const auto res = root->execute();
        REQUIRE(std::holds_alternative<int>(res));
        REQUIRE(std::get<int>(res) == -12);
    }

    SECTION("Unary bang and NOT: !0 == 1, !5 == 0, NOT 0 == 1") {
        {
            constexpr std::string_view input = "select ( !0 );";
            hivedb::lexer l{input};
            hivedb::parser ast{l.getTokens()};
            const auto res = ast.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( !5 );";
            hivedb::lexer l{input};
            hivedb::parser ast{l.getTokens()};
            const auto res = ast.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
        {
            constexpr std::string_view input = "select ( NOT 0 );";
            hivedb::lexer l{input};
            hivedb::parser ast{l.getTokens()};
            const auto res = ast.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
    }
}

TEST_CASE("Parser - Comparison expressions evaluation", "[parser]") {
    SECTION("Equality = and ==") {
        {
            constexpr std::string_view input = "select ( 5 == 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 5 = 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 5 == 6 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
    }

    SECTION("Inequality !=") {
        {
            constexpr std::string_view input = "select ( 5 != 6 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 5 != 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
    }

    SECTION("Relational <, <=, >, >=") {
        {
            constexpr std::string_view input = "select ( 3 < 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 5 <= 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 6 <= 5 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
        {
            constexpr std::string_view input = "select ( 7 > 4 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 4 >= 4 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 3 >= 4 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
    }

    SECTION("String comparisons") {
        {
            constexpr std::string_view input = "select ( \"abc\" == \"abc\" );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( \"abc\" != \"def\" );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( \"abc\" < \"def\" );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
    }
}

TEST_CASE("Parser - Logical AND / OR expressions evaluation", "[parser]") {
    SECTION("AND and OR basics") {
        {
            constexpr std::string_view input = "select ( 1 AND 1 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 1 AND 0 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
        {
            constexpr std::string_view input = "select ( 0 OR 1 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 1);
        }
        {
            constexpr std::string_view input = "select ( 0 OR 0 );";
            hivedb::lexer l{input};
            const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
            REQUIRE(std::get<int>(res) == 0);
        }
    }

    SECTION("Comparison has higher precedence than AND / OR") {
        constexpr std::string_view input = "select ( 5 > 2 AND 3 < 10 );";
        hivedb::lexer l{input};
        const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
        REQUIRE(std::get<int>(res) == 1);
    }

    SECTION("AND has higher precedence than OR: 1 OR 0 AND 0 == 1") {
        constexpr std::string_view input = "select ( 1 OR 0 AND 0 );";
        hivedb::lexer l{input};
        const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
        REQUIRE(std::get<int>(res) == 1);
    }

    SECTION("Parentheses override AND/OR precedence: (1 OR 0) AND 0 == 0") {
        constexpr std::string_view input = "select ( (1 OR 0) AND 0 );";
        hivedb::lexer l{input};
        const auto res = hivedb::parser{l.getTokens()}.parse()->execute();
        REQUIRE(std::get<int>(res) == 0);
    }
}

TEST_CASE("Parser - WHERE clause parsing and AST structure", "[parser]") {
    SECTION("Simple WHERE clause") {
        constexpr std::string_view input = "select id, name from users where id > 10;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName == "users");
        REQUIRE(sel->innerExpr.size() == 2);
        REQUIRE(sel->whereExpr != nullptr);

        const auto* whereBin = dynamic_cast<const hivedb::binary_expr*>(sel->whereExpr.get());
        REQUIRE(whereBin != nullptr);
        REQUIRE(whereBin->op == hivedb::token_type::greater);

        const auto cols = sel->retriveColumnsToBeFetched();
        REQUIRE(cols.size() == 3);
        REQUIRE(cols[0] == "id");
        REQUIRE(cols[1] == "name");
        REQUIRE(cols[2] == "id");
    }

    SECTION("Compound WHERE clause with AND and OR") {
        constexpr std::string_view input =
            "select * from users where age >= 18 and status == \"active\" or role == \"admin\";";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName == "users");
        REQUIRE(sel->whereExpr != nullptr);

        // Precedence: OR at root of where, LHS is AND, RHS is ==
        const auto* whereOr = dynamic_cast<const hivedb::binary_expr*>(sel->whereExpr.get());
        REQUIRE(whereOr != nullptr);
        REQUIRE(whereOr->op == hivedb::token_type::_or);

        const auto* whereAnd = dynamic_cast<const hivedb::binary_expr*>(whereOr->lhs.get());
        REQUIRE(whereAnd != nullptr);
        REQUIRE(whereAnd->op == hivedb::token_type::_and);

        const auto cols = sel->retriveColumnsToBeFetched();
        REQUIRE(cols.size() == 4);
        REQUIRE(cols[0] == "*");
        REQUIRE(cols[1] == "age");
        REQUIRE(cols[2] == "status");
        REQUIRE(cols[3] == "role");
    }

    SECTION("WHERE clause without FROM clause") {
        constexpr std::string_view input = "select ( 100 ) where 5 > 2;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        const auto root = ast.parse();

        REQUIRE(root != nullptr);
        const auto* sel = dynamic_cast<const hivedb::select_expr*>(root.get());
        REQUIRE(sel != nullptr);
        REQUIRE(sel->tblName.empty());
        REQUIRE(sel->whereExpr != nullptr);

        const auto whereRes = sel->whereExpr->execute();
        REQUIRE(std::holds_alternative<int>(whereRes));
        REQUIRE(std::get<int>(whereRes) == 1);
    }
}

TEST_CASE("Parser - Malformed syntax errors", "[parser]") {
    SECTION("Missing table in from clause") {
        constexpr std::string_view input = "select ( 1 ) from;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
    }

    SECTION("Create table missing column name") {
        constexpr std::string_view input = "create table foo ();";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
    }

    SECTION("Insert missing INTO keyword") {
        constexpr std::string_view input = "insert foo (bar) values (1);";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
    }

    SECTION("Insert missing VALUES keyword") {
        constexpr std::string_view input = "insert into foo (bar) (1);";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
    }

    SECTION("Unclosed parenthesis in projection") {
        constexpr std::string_view input = "select ( 1 + 2 ;";
        hivedb::lexer l{input};
        hivedb::parser ast{l.getTokens()};
        REQUIRE_THROWS_AS(ast.parse(), std::invalid_argument);
    }
}
