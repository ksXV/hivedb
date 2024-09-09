#include <stdexcept>
#include <cassert>
#include <vector>

#include <fmt/core.h>

#include <parser/tokens.h>
#include <parser/ast.h>

//TODO: needs more work
SelectStatement AST::parseSelectStatement() {
    SelectStatement ss{};
    auto token = lexer.nextToken();
    assert(token.getTokenType() != TokenType::illegal);

    std::vector<std::string_view> columns{};

    while (true) {
        if (token.getTokenType() == TokenType::eof || token.getTokenType() == TokenType::from) {
            break;
        }
        if (token.getTokenType() == TokenType::identifier) {
            std::string_view literal = std::get<std::string_view>(token.getLiteral());
            columns.push_back(literal);
        }
        token = lexer.nextToken();

        assert(token.getTokenType() != TokenType::illegal && "GOT ILEGAL");
    }

    if (token.getTokenType() == TokenType::eof) {
        ss = SelectClause{columns};
        return ss;
    }
    assert(token.getTokenType() == TokenType::from);

    token = lexer.nextToken();
    std::string_view tableName = std::get<std::string_view>(token.getLiteral());

    token = lexer.nextToken();
    if (token.getTokenType() == TokenType::eof) {
        ss = {columns, tableName};
        return ss;
    }

    return ss;
}

void AST::startParsing() {
    auto token = lexer.nextToken();

    if (token.getTokenType() == TokenType::select) {
        root = parseSelectStatement();
    } else {
        //TODO: do this for later
        throw std::runtime_error(fmt::format("NO IMPLEMENTATION FOR {}\n", token));
    }

}
