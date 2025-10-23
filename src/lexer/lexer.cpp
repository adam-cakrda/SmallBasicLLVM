#include "lexer.hpp"

#include <algorithm>
#include <iostream>

char Lexer::eat() {
    const char c = this->peek();
    if (c == '\0') return c;

    if (c == '\n') {
        this->line++;
        this->col = 0;
    } else {
        this->col++;
    }
    this->pos++;
    return c;
}

char Lexer::peek(const size_t offset) const {
    if (this->pos + offset >= this->source.length()) return '\0';
    return this->source[pos + offset];
}

void Lexer::skipIgnored() {
    bool skippedSomething;
    do {
        skippedSomething = false;

        while (std::isspace(this->peek())) {
            this->eat();
            skippedSomething = true;
        }

        if (this->peek() == '\'') {
            while (this->peek() != '\n' && this->peek() != '\0') {
                this->eat();
            }
            skippedSomething = true;
        }
    } while (skippedSomething);
}

Token Lexer::makeString() {
    const size_t startColumn = this->col;
    const size_t startLine = this->line;
    this->eat();

    std::string value;
    while (this->peek() != '"' && this->peek() != '\0') {
        if (this->peek() == '\n') {
            this->reporter->addError("unterminated string literal",
                SourceLocation(startLine, startColumn, 1),
                "strings cannot span multiple lines");
            break;
        }
        value += this->eat();
    }

    if (this->peek() == '"') {
        this->eat(); // skip closing "
    } else if (this->peek() == '\0') {
        this->reporter->addError("unterminated string literal",
            SourceLocation(startLine, startColumn, 1),
            "expected closing `\"`");
    }

    return {TokenTyp::StringLiteral, value, startLine, startColumn};
}

Token Lexer::makeNumber() {
    const size_t startColumn = this->col;
    const size_t startLine = this->line;
    std::string value;

    bool seenDot = false;
    while (std::isdigit(peek()) || (peek() == '.' && !seenDot)) {
        if (peek() == '.') seenDot = true;
        value += eat();
    }

    return {TokenTyp::NumberLiteral, value, startLine, startColumn};
}

Token Lexer::makeIdentifier() {
    const size_t startColumn = this->col;
    const size_t startLine = this->line;
    std::string value;

    while (std::isalnum(this->peek()) || this->peek() == '_') {
        value += this->eat();
    }

    std::string lowerValue = value;
    std::ranges::transform(lowerValue, lowerValue.begin(),
                           [](const unsigned char c) { return std::tolower(c); });

    if (this->keywords.contains(lowerValue)) {
        return {this->keywords[lowerValue], value, startLine, startColumn};
    }

    return {TokenTyp::Identifier, value, startLine, startColumn};
}


Token Lexer::makeTwoCharOperator() {
    const size_t startColumn = this->col;
    const size_t startLine = this->line;
    std::string current;
    current += this->eat();
    current += this->eat();

    return {this->twoCharSymbols[current], current, startLine, startColumn};
}

Token Lexer::makeSingleCharOperator() {
    const size_t startColumn = this->col;
    const size_t startLine = this->line;
    std::string current(1, this->eat());

    return {this->symbols[current], current, startLine, startColumn};
}

std::vector<Token> Lexer::tokenize(const std::string &input, DiagnosticReporter& diag) {
    this->source = input;
    this->pos = 0;
    this->line = 1;
    this->col = 0;
    this->reporter = &diag;

    std::vector<Token> tokens;
    while (this->peek() != '\0') {
        this->skipIgnored();

        if (this->peek() == '\0') break;

        if (peek() == '"') {
            tokens.push_back(this->makeString());
            continue;
        }

        if (std::isdigit(peek())) {
            tokens.push_back(this->makeNumber());
            continue;
        }

        if (std::isalpha(peek()) || peek() == '_') {
            tokens.push_back(this->makeIdentifier());
            continue;
        }

        std::string twoChar(1, this->peek());
        twoChar += peek(1);

        if (this->twoCharSymbols.contains(twoChar)) {
            tokens.push_back(this->makeTwoCharOperator());
            continue;
        }

        std::string singleChar(1, this->peek());
        if (this->symbols.contains(singleChar)) {
            tokens.push_back(this->makeSingleCharOperator());
            continue;
        }

        reporter->addError("unexpected character: '" + singleChar + "'",
            SourceLocation(line, col, 1),
            "this character is not valid in this context");
        this->eat();
    }

    return tokens;
}
