#include <parser/tokens.h>

#include <iostream>
#include <variant>

std::ostream& operator<<(std::ostream& os, const Token& t) {
    const auto l = [&](auto& arg){os << arg;};
    std::visit(l, t.literal);
    os << " " << static_cast<int>(t.type);
    return os;
}
