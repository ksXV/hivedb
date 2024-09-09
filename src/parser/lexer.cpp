#include <cctype>
#include <stdexcept>
#include <string_view>

#include <parser/lexer.h>
#include <parser/tokens.h>

void Lexer::readChar() noexcept {
        this->currentChar = this->readPosition >= this->input.length() ?
                    '\0' : this->input.at(readPosition);
        this->position = this->readPosition;
        ++this->readPosition;
        return;
}

std::string_view Lexer::getIdentifier() noexcept {
    std::size_t currentPosition = position;
    //skip "
    readChar();
    //advance the cursor
    getWord();
    //skip " again
    readChar();

    return std::string_view{input.c_str() + currentPosition, position - currentPosition};
}


Token Lexer::nextToken() {
    Token t;

    if (input.empty()) {
        throw std::runtime_error("I FUCKED UP");
    }

    if (position == 0 && !currentChar) {
        currentChar = input.at(0);
    }

    skipWhiteSpaces();

    switch (currentChar) {
        case '(':
            t = {TokenType::parenthesesL, '('};
            break;
        case ')':
            t = {TokenType::parenthesesR, ')'};
            break;
        case '+':
            t = {TokenType::plusOperator, '+'};
            break;
        case '\0':
            t = {TokenType::eof, '\0'};
            break;
        default:
            if (std::isalpha(currentChar)) {
                auto word = getWord();
                if (word == "select") {
                    t = {TokenType::select, word};
                } else if (word == "from") {
                    t = {TokenType::from, word};
                } else {
                    t = {TokenType::identifier, word};
                }
            } else if (currentChar == '"') {
                t = {TokenType::identifier, getIdentifier()};
            } else {
                t = {TokenType::illegal, '\0'};
            }
            break;
    }
    this->readChar();

    return t;
}

Token Lexer::getNextToken(TokenType t) const noexcept {
    return Token{t, this->currentChar};
}

void Lexer::skipWhiteSpaces() noexcept {
    while (this->currentChar == ' ' || this->currentChar == '\t' || this->currentChar == '\r' || this->currentChar == '\n') {
            this->readChar();
        }
}

std::string_view Lexer::getWord() noexcept {
    std::size_t currentPosition = this->position;

    while (std::isalpha(this->currentChar)) {
        this->readChar();
    }

    return std::string_view{this->input.c_str() + currentPosition, this->position - currentPosition};
}
