#include <cctype>
#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <parser/lexer.hpp>
#include <parser/tokens.hpp>

namespace hivedb {

using namespace std::literals;
const std::unordered_map<std::string_view, token_type> lexer::reservedKeywords{
    {"select"sv, token_type::select},
    {"from"sv, token_type::from},
    {"create"sv, token_type::create},
    {"table"sv, token_type::table},
    {"not"sv, token_type::_not},
    {"null"sv, token_type::null},
    {"insert"sv, token_type::insert},
    {"into"sv, token_type::into},
    {"values"sv, token_type::values},
};

lexer::lexer(std::string_view input):
    m_input{input}, m_position{0}, m_nextPosition{1}, m_current{} {
    if (input.empty()) {
        throw std::invalid_argument("bad input");
    }
    m_current = input[m_position];
};

bool lexer::isLetter(char c) {
   return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool lexer::isDigit(char c) {
   return (c >= '0' && c <= '9');
}

bool lexer::isAtEnd() const noexcept {
    if (m_nextPosition > m_input.length()) return true;
    return false;
}

void lexer::read() noexcept {
        m_current = isAtEnd() ?
                    '\0' : m_input[m_nextPosition];
        m_position = m_nextPosition++;
}

char lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return m_input[m_nextPosition];
}


void lexer::addToken(token_type type, std::string_view identifier = "") noexcept {
   m_tokens.emplace_back(type, identifier);
}

void lexer::nextToken() {
    assert(!m_input.empty());

    switch (m_current) {
        case '(':
            addToken(token_type::parenthesesL);
            break;
        case ')':
            addToken(token_type::parenthesesR);
            break;
        case '+':
            addToken(token_type::add);
            break;
        case '-':
            addToken(token_type::substract);
            break;
        case '/':
            addToken(token_type::divide);
            break;
        case '*':
            addToken(token_type::star);
            break;
        case ',':
            addToken(token_type::comma);
            break;
        case '.':
            addToken(token_type::dot);
            break;
        case '!':
            addToken(token_type::bang);
            break;
        case '\0': [[fallthrough]];
        case ';':
            addToken(token_type::eof);
            break;
        case '"':
            parseString();
            break;

        case ' ':   [[fallthrough]];
        case '\n':  [[fallthrough]];
        case '\r':  [[fallthrough]];
        case '\t':  break;

        default:
            if (isLetter(m_current)) {
                addIdentifier();
            } else if (isDigit(m_current)) {
                addNumber();
            } else {
                throw std::invalid_argument("unknown identifier");
            }
            break;
    }

    read();
}

void lexer::addNumber() {
    const std::size_t currentPos = m_position;
    while (isDigit(peek())) read();

    if (m_current == '.' && isDigit(peek())) {
        while (isDigit(peek())) read();

        const std::string_view number{m_input.c_str() + currentPos, m_position - currentPos + 1};

        addToken(token_type::real, number);
        return;
    }

    const std::string_view number{m_input.c_str() + currentPos, m_position - currentPos + 1};

    addToken(token_type::integer, number);
}

void lexer::parseString() {
    //skip the first '"'
    read();

    const std::size_t currentPos = m_position;

    while (!isAtEnd() && peek() != '\"') {
        read();
    }
    const std::string_view str{m_input.c_str() + currentPos, m_position - currentPos + 1};

    //skip the second "
    read();

    addToken(token_type::string, str);
}

void lexer::addIdentifier() {
    const std::size_t currentPos = m_position;
    while (isLetter(peek())) {
        read();
    }

    const std::string_view keyword{m_input.c_str() + currentPos, m_position - currentPos + 1};

    if (auto it = reservedKeywords.find(keyword); it != reservedKeywords.end()) {
       const auto [i, t] = *it;
       addToken(t, i);
       return;
    }

    addToken(token_type::identifier, keyword);
}

std::vector<token> lexer::getTokens() {
    while (m_current != '\0') {
        nextToken();
    }

    return m_tokens;
}
}
