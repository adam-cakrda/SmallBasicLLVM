#pragma once
#include <map>
#include <string>
#include <vector>

#include "token.hpp"
#include "../diagnostic.hpp"

class Lexer {
public:
    Lexer(): reporter(nullptr), pos(0), line(1), col(0) {}
    std::vector<Token> tokenize(const std::string& input, DiagnosticReporter& diag);
private:
    void skipIgnored();

    Token makeString();
    Token makeNumber();
    Token makeIdentifier();
    Token makeTwoCharOperator();
    Token makeSingleCharOperator();

    [[nodiscard]] char peek(size_t offset = 0) const;
    char eat();

    DiagnosticReporter* reporter;
    std::string source;
    size_t pos;
    size_t line;
    size_t col;

    std::map<std::string, TokenTyp> keywords = {
        {"if", TokenTyp::If},
        {"then", TokenTyp::Then},
        {"else", TokenTyp::Else},
        {"elseif", TokenTyp::ElseIf},
        {"endif", TokenTyp::EndIf},
        {"for", TokenTyp::For},
        {"to", TokenTyp::To},
        {"step", TokenTyp::Step},
        {"endfor", TokenTyp::EndFor},
        {"while", TokenTyp::While},
        {"endwhile", TokenTyp::EndWhile},
        {"sub", TokenTyp::Sub},
        {"endsub", TokenTyp::EndSub},
        {"goto", TokenTyp::GoTo},
        {"or", TokenTyp::Or},
        {"and", TokenTyp::And}
    };

    std::map<std::string, TokenTyp> symbols = {
        {".", TokenTyp::Dot},
        {",", TokenTyp::Comma},
        {"(", TokenTyp::LeftParen},
        {")", TokenTyp::RightParen},
        {"[", TokenTyp::LeftBracket},
        {"]", TokenTyp::RightBracket},
        {"=", TokenTyp::Equal},
        {"+", TokenTyp::Plus},
        {"-", TokenTyp::Minus},
        {"*", TokenTyp::Multiply},
        {"/", TokenTyp::Divide},
        {":", TokenTyp::Colon},
        {"<", TokenTyp::LessThan},
        {">", TokenTyp::GreaterThan}
    };

    std::map<std::string, TokenTyp> twoCharSymbols = {
        {"<=", TokenTyp::LessThanOrEqual},
        {">=", TokenTyp::GreaterThanOrEqual},
        {"<>", TokenTyp::NotEqual}
    };
};
