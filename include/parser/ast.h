#pragma once

#include "parser/lexer.h"

#include <vector>
#include <memory>

enum class StatementType {
    select,
    insert,
    createTable,
    //TODO: add more types
};

class Statement {
    private:
        [[maybe_unused]] const StatementType typeOfStatement;
    public:
        constexpr Statement(StatementType s) noexcept : typeOfStatement{s} {}
};

class SelectStatement: Statement {

};

class ASTNode {
    private:
        std::vector<std::unique_ptr<ASTNode>> childern;
        std::unique_ptr<Statement> statement;
    public:
        ASTNode() noexcept : childern{} {}
        void addNode(ASTNode&) noexcept;
};

class AST {
    private:
        ASTNode root;
        Lexer lexer;
};
