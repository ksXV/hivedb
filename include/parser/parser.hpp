#pragma once

#include <parser/tokens.hpp>
#include <memory>
#include <parser/lexer.hpp>
#include <sstream>

#define EXPR_INIT(name)                \
name() = default;                      \
name(const name&) = delete;            \
name& operator=(const name&) = delete; \
name(name&&) = default;                \
name& operator=(name&&) = default;     \
~name() override = default;

namespace hivedb {
struct Expr {
    virtual void prettyPrint(std::stringstream& s) const = 0;
    virtual ~Expr() = default;
};

template <typename T>
struct LiteralExpr final: public Expr {
    T value;

    explicit LiteralExpr(T&& v): value(v) {};

    void prettyPrint(std::stringstream& s) const override {
       s << "( literal: " << value << " )";
    }
};

struct UnaryExpr final : public Expr {
    TokenType op{};
    std::unique_ptr<Expr> rhs{};

    void prettyPrint(std::stringstream& s) const override {
        if (op == TokenType::bang) s << "!";
        if (op == TokenType::substract) s << "-";
        rhs->prettyPrint(s);
    }

    EXPR_INIT(UnaryExpr)
};

struct GroupingExpr final: public Expr {
    std::unique_ptr<Expr> expr;

    void prettyPrint(std::stringstream& s) const override {
            s << "( grouping: ";
            expr->prettyPrint(s);
            s << ")";
    };

    EXPR_INIT(GroupingExpr)
};

struct BinaryExpr final: public Expr {
    std::unique_ptr<Expr> lhs;
    TokenType op{};
    std::unique_ptr<Expr> rhs;

    void prettyPrint(std::stringstream& s) const override {
        s << "binary: ( ";
        lhs->prettyPrint(s);
        if (op == TokenType::add) s << " + ";
        if (op == TokenType::substract) s << " - ";
        if (op == TokenType::divide) s << " / ";
        if (op == TokenType::star) s << " * ";
        rhs->prettyPrint(s);
        s << ") ";
    };

    EXPR_INIT(BinaryExpr)
};

struct CommaExpr final: public Expr {
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    void prettyPrint(std::stringstream& s) const override {
        s << "comma: (";
        lhs->prettyPrint(s);
        s << ", ";
        rhs->prettyPrint(s);
        s << ")";
    };

    EXPR_INIT(CommaExpr)
};

struct Column {
    std::string_view name;
    std::string_view type;
    bool canBeNull;
};

struct CreateTblExpr final: public Expr {
    std::string_view tblName;
    std::vector<Column> tblColumns;

    void prettyPrint(std::stringstream& s) const override {
        s << "Table name: " << tblName << "\n";
        for (auto& c : tblColumns) {
            s << "Name: " << c.name << " Type: " << c.type << " Null?: " << c.canBeNull;
            s << "\n";
        }
    }

    EXPR_INIT(CreateTblExpr)
};

struct InsertExpr final: public Expr {
    std::string_view tblName;
    std::vector<std::string_view> columns;
    std::vector<std::string_view> values;

    void prettyPrint(std::stringstream& s) const override {
        s << "insert into: " << tblName << "values: \n";
        for (std::size_t i = 0; i < columns.size(); ++i) {
            s << columns[i] << " value -> " << values[i] << '\n';
        }
    }

    EXPR_INIT(InsertExpr)
};

struct SelectExpr final: public Expr {
   std::unique_ptr<Expr> selectExpr;
   struct {
       std::string_view tableName;
       std::string_view databaseName;
   } tableExpr;

   // TODO: Replace this later
   std::unique_ptr<Expr> whereExpr;

   void prettyPrint(std::stringstream& s) const override {
        s << "select: (";
        selectExpr->prettyPrint(s);
        s << ")";
        s << " from table: " << (tableExpr.databaseName.length() ? tableExpr.databaseName : "NULL");
        s << "." << (tableExpr.tableName.length() ? tableExpr.tableName : "NULL") << "\n";

        s << "\n";
   };

   EXPR_INIT(SelectExpr)
};

class Parser {
   private:
   std::vector<Token> m_tokens;

   std::size_t m_cursor{0};

    [[nodiscard]]
   inline const Token& peek() const noexcept;
    [[nodiscard]]
   inline const Token& previous() const noexcept;
    [[nodiscard]]
   inline const Token& current() const noexcept;

   inline const Token& advance() noexcept;

   template <std::same_as<TokenType>... T>
   bool match(T...) noexcept;

    [[nodiscard]]
   inline bool isDone() const noexcept;
    [[nodiscard]]
   inline bool check(TokenType) const noexcept;

   inline void consume(TokenType, std::string_view);

    [[nodiscard]]
   inline std::unique_ptr<Expr> expr();

    [[nodiscard]]
   inline std::unique_ptr<Expr> commaExpr();

    [[nodiscard]]
   inline std::unique_ptr<Expr> binaryExpr();

    [[nodiscard]]
   inline std::unique_ptr<Expr> unaryExpr();

    [[nodiscard]]
   inline std::unique_ptr<Expr> primaryExpr();

    [[nodiscard]]
   inline std::unique_ptr<Expr> selectExpr();

   [[nodiscard]]
   inline std::unique_ptr<Expr> createExpr();

   [[nodiscard]]
   inline std::unique_ptr<Expr> insertExpr();

   public:
   explicit Parser(const std::vector<Token>& t);

   std::unique_ptr<Expr> parse();

   Parser(const Parser&) = delete;
   Parser& operator=(const Parser&) = delete;

   Parser(Parser&&) = default;
   Parser& operator=(Parser&&) = default;

   ~Parser() = default;
};

}
