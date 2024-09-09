#include <iostream>
#include <variant>

#include <parser/tokens.h>

std::ostream& operator<<(std::ostream& os, const Token& t) {
    const auto l = [&](const auto& arg){
            os << arg;
    };
    std::visit(l, t.literal);
    os << " " << static_cast<int>(t.type);
    return os;
}

std::string Token::name() const {
    std::string n;

    const auto l = [&](const auto& arg){
        n = arg;
    };
    std::visit(l, literal);

    n += " " + std::to_string(static_cast<int>(type));
    return n;
}
