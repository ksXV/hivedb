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
const std::unordered_map<std::string_view, TokenType> Lexer::reservedKeywords{
    {"select"sv, TokenType::select},
    {"from"sv, TokenType::from},
    {"create"sv, TokenType::create},
    {"table"sv, TokenType::table},
    {"not"sv, TokenType::_not},
    {"null"sv, TokenType::null},
    {"insert"sv, TokenType::insert},
    {"into"sv, TokenType::into},
    {"values"sv, TokenType::values},
};

Lexer::Lexer(std::string_view input):
    m_input{input}, m_position{0}, m_nextPosition{1}, m_current{} {
    if (input.empty()) {
        throw std::invalid_argument("bad input");
    }
    m_current = input[m_position];
};

bool Lexer::isLetter(char c) {
   return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool Lexer::isDigit(char c) {
   return (c >= '0' && c <= '9');
}

bool Lexer::isAtEnd() const noexcept {
    if (m_nextPosition > m_input.length()) return true;
    return false;
}

void Lexer::read() noexcept {
        m_current = isAtEnd() ?
                    '\0' : m_input[m_nextPosition];
        m_position = m_nextPosition++;
}

char Lexer::peek() const noexcept {
    if (isAtEnd()) return '\0';
    return m_input[m_nextPosition];
}


void Lexer::addToken(TokenType type, std::string_view identifier = "") noexcept {
   m_tokens.emplace_back(type, identifier);
}

void Lexer::nextToken() {
    assert(!m_input.empty());

    switch (m_current) {
        case '(':
            addToken(TokenType::parenthesesL);
            break;
        case ')':
            addToken(TokenType::parenthesesR);
            break;
        case '+':
            addToken(TokenType::add);
            break;
        case '-':
            addToken(TokenType::substract);
            break;
        case '/':
            addToken(TokenType::divide);
            break;
        case '*':
            addToken(TokenType::star);
            break;
        case ',':
            addToken(TokenType::comma);
            break;
        case '.':
            addToken(TokenType::dot);
            break;
        case '!':
            addToken(TokenType::bang);
            break;
        case '\0': [[fallthrough]];
        case ';':
            addToken(TokenType::eof);
            break;
        case '"':
            addToken(TokenType::quote);
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

void Lexer::addNumber() {
    const std::size_t currentPos = m_position;
    while (isDigit(peek())) read();

    if (m_current == '.' && isDigit(peek())) {
        while (isDigit(peek())) read();

        const std::string_view number{m_input.c_str() + currentPos, m_position - currentPos + 1};

        addToken(TokenType::real, number);
        return;
    }

    const std::string_view number{m_input.c_str() + currentPos, m_position - currentPos + 1};

    addToken(TokenType::integer, number);
}

void Lexer::addIdentifier() {
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

    addToken(TokenType::identifier, keyword);
}

std::vector<Token> Lexer::getTokens() {
    while (m_current != '\0') {
        nextToken();
    }

    return m_tokens;
}
}
