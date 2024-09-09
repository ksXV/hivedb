#pragma once

#include <string_view>
#include <vector>

#include <parser/lexer.h>

class SelectClause {
    private:
        std::vector<std::string_view> columns;
    public:
        SelectClause(std::vector<std::string_view>& c): columns{c} {}
        SelectClause() noexcept: columns{} {}
};

class FromClause {
    private:
        std::string_view table;
    public:
        constexpr FromClause(std::string_view t) noexcept: table{t} {}
        constexpr FromClause() noexcept {}
};

class SelectStatement {
    private:
        SelectClause sc;
        FromClause fc;
        //TODO: add more
    public:
        SelectStatement(SelectClause c, FromClause f): sc{c}, fc{f} {};
        SelectStatement(SelectClause c): sc{c} {};
        SelectStatement() {};
};

class AST {
    private:
        SelectStatement root;
        Lexer lexer;

        SelectStatement parseSelectStatement();
        void startParsing();
    public:
        AST(std::string_view i) noexcept: root{}, lexer{i} {startParsing();};
        auto getRoot() const noexcept { return root; }
};
