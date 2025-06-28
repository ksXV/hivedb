#include <cmath>
#include <memory>
#include <cassert>
#include <catch_amalgamated.hpp>

#include <parser/tokens.hpp>
#include <parser/parser.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace hivedb {
    Parser::Parser(const std::vector<Token>& t): m_tokens(t) {}

    const Token& Parser::current() const noexcept {
        assert(m_cursor <= m_tokens.size());
        return m_tokens[m_cursor];
    }

    const Token& Parser::previous() const noexcept {
        assert(m_cursor != 0);
        return m_tokens[m_cursor-1];
    }

    const Token& Parser::peek() const noexcept {
        assert(m_cursor+1 <= m_tokens.size());
        return m_tokens[m_cursor+1];
    }

    bool Parser::isDone() const noexcept {
       return m_tokens.size() <= m_cursor;
    }

    const Token& Parser::advance() noexcept {
        if (!isDone()) m_cursor++;
        return previous();
    }

    bool Parser::check(TokenType type) const noexcept {
       if (isDone()) return false;
       return current().type == type;
    }


    template <std::same_as<TokenType>... T>
    bool Parser::match(T... types) noexcept {
        return (... || check(types)) && (advance(), true);
    }

    void Parser::consume(TokenType type, std::string_view err) {
        if (match(type)) return;
        throw std::invalid_argument(err.data());
    }

    std::unique_ptr<Expr> Parser::commaExpr() {
       auto e = unaryExpr();
       if (match(TokenType::comma)) {
           auto c = std::make_unique<CommaExpr>();
           c->lhs = std::move(e);
           c->rhs = binaryExpr();

           return c;
       }

       return e;
    }

    std::unique_ptr<Expr> Parser::binaryExpr() {
       auto e = commaExpr();
       if (match(TokenType::add, TokenType::substract, TokenType::divide, TokenType::star)) {
           auto b = std::make_unique<BinaryExpr>();
           b->lhs = std::move(e);
           b->op = previous().type;
           b->rhs = binaryExpr();

           return b;
       }

       return e;
    }

    std::unique_ptr<Expr> Parser::unaryExpr() {
       if (match(TokenType::substract, TokenType::bang)) {
           auto e = std::make_unique<UnaryExpr>();
           e->op = previous().type;
           e->rhs = unaryExpr();
           return e;
       }

       return primaryExpr();
    }

    std::unique_ptr<Expr> Parser::primaryExpr() {
        if (match(TokenType::quote)) {
            const auto literal = current().literal;

            auto e = std::make_unique<LiteralExpr<std::string>>(std::string{literal.data(), literal.size()});

            advance();

            consume(TokenType::quote, "Unfinished string.");
            return e;
        }

        if (match(TokenType::integer)) {
            const auto literal = previous().literal;

            // TODO: remove this dumb allocation...
            auto e = std::make_unique<LiteralExpr<int>>(std::stoi(literal.data()));
            return e;
        }

        if (match(TokenType::real)) {
            const auto literal = previous().literal;
            // TODO: also remove this dumb allocation...
            auto e = std::make_unique<LiteralExpr<double>>(std::stod(literal.data()));
            return e;
        }

        if (match(TokenType::parenthesesL)) {
            auto e = match(TokenType::select) ? selectExpr() : binaryExpr();

            consume(TokenType::parenthesesR, "Missing parentheses.");

            auto g = std::make_unique<GroupingExpr>();
            g->expr = std::move(e);
            return g;
        }

       throw std::invalid_argument("Invalid token: " + std::string(current().name()));
    }

    std::unique_ptr<Expr> Parser::expr() {
       if (match(TokenType::select)) {
          return selectExpr();
       } else if (match(TokenType::create)) {
          return createExpr();
       } else if (match(TokenType::insert)) {
          return insertExpr();
       }
       throw std::invalid_argument("Invalid token: " + std::string(current().name()));
    }

    std::unique_ptr<Expr> Parser::insertExpr() {
        auto e = std::make_unique<InsertExpr>();
        if (!match(TokenType::into)) throw std::invalid_argument("Invalid insert stmt! Missing INTO.");

        if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid insert stmt! Where do i insert into?");
        e->tblName = previous().literal;

        if (!match(TokenType::parenthesesL)) throw std::invalid_argument("Invalid insert stmt! Missing \"(\" !");

        while (!isDone()) {
            if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid insert stmt! Missing column!");
            std::string_view column = previous().literal;

            e->columns.push_back(column);

            if (match(TokenType::comma)) {
                continue;
            }
            if (match(TokenType::parenthesesR)) {
                break;
            }

            throw std::invalid_argument("SHOULDNT REACH THIS!");
        }

        if (!match(TokenType::values)) throw std::invalid_argument("Invalid insert stmt! Missing values!!!");

        if (!match(TokenType::parenthesesL)) throw std::invalid_argument("Invalid insert stmt! Missing values \"(\" !");

        while (!isDone()) {
            if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid insert stmt! Missing column!");
            std::string_view column = previous().literal;

            e->columns.emplace_back(column);

            if (match(TokenType::comma)) {
                continue;
            }
            if (match(TokenType::parenthesesR)) {
                break;
            }

            throw std::invalid_argument("SHOULDNT REACH THIS!");
        }

        return e;
    }

    std::unique_ptr<Expr> Parser::createExpr() {
        auto e = std::make_unique<CreateTblExpr>();

        if (!match(TokenType::table)) throw std::invalid_argument("Invalid create stmt! Missing TABLE.");

        if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid create stmt! Missing identifier for table name.");

        e->tblName = previous().literal;

        if (!match(TokenType::parenthesesL)) throw std::invalid_argument("Invalid create stmt! Missing parantheses!");

        while (!isDone()) {
            if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid create stmt! missing column name!");
            std::string_view tblName = previous().literal;
            if (!match(TokenType::identifier)) throw std::invalid_argument("Invalid create stmt! missing type for column: " + std::string(tblName));
            std::string_view type = previous().literal;

            bool canBeNull = false;
            if (match(TokenType::_not)) {
                if (!match(TokenType::null)) throw std::invalid_argument("Invalid create stmt! Wtf are you trying to negate.");
                canBeNull = true;
            }
            e->tblColumns.emplace_back(tblName, type, canBeNull);
            if (match(TokenType::comma)) {
                continue;
            }
            if (match(TokenType::parenthesesR)) {
                break;
            }

            throw std::invalid_argument("SHOULDNT REACH THIS!");
        }
        return e;
    }

    std::unique_ptr<Expr> Parser::selectExpr() {
        auto e = binaryExpr();
        auto s = std::make_unique<SelectExpr>();
        s->selectExpr = std::move(e);

        if (match(TokenType::from)) {
            if (current().type != TokenType::identifier) {
                throw std::invalid_argument("Invalid token detected!");
            }

            if (peek().type == TokenType::dot) {
                s->tableExpr.databaseName = current().literal;
                advance();
                // skip the dot
                advance();
            }

            s->tableExpr.tableName = current().literal;
        }

        // TODO: continue for where here
        return s;
    }


    std::unique_ptr<Expr> Parser::parse() {
        std::unique_ptr<Expr> e = expr();

        return e;
    }
}
