#include <iostream>

#include <parser/tokens.hpp>

namespace hivedb {

Token::Token(TokenType t): type(t), literal() {}
Token::Token(TokenType t, std::string_view l): type(t), literal(l) {}

bool operator==(const Token& lhs, const Token& rhs) noexcept {
        return lhs.literal == rhs.literal && lhs.type == rhs.type;
}

static constexpr std::string_view getTokenType(TokenType t) {
    switch (t) {
        case TokenType::illegal:         return "illegal";
        case TokenType::eof:             return "eof";
        case TokenType::parenthesesL:    return "parenthesesL";
        case TokenType::parenthesesR:    return "parenthesesR";
        case TokenType::add:             return "add";
        case TokenType::select:          return "select";
        case TokenType::from:            return "from";
        case TokenType::identifier:      return "identifier";
        case TokenType::quote:           return "quote";
        case TokenType::bang:            return "bang";
        case TokenType::comma:           return "comma";
        case TokenType::dot:             return "dot";
        default:                         return "unknown yet";
    }
}

std::ostream& operator<<(std::ostream& os, const Token& t) {
    os << "{ ";
    if (!t.literal.empty()) {
        os << t.literal << " ";
    }
    os << getTokenType(t.type);
    os << " }";
    return os;
}

std::string Token::name() const {
    if (literal.empty()) {
        return std::string{getTokenType(type)};
    }

    return std::string{literal} + " " + std::string{getTokenType(type)};
}
}
