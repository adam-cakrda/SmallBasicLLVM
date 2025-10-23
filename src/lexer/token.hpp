#pragma once
#include <string>

enum class TokenTyp {
    If,
    Then,
    Else,
    ElseIf,
    EndIf,
    For,
    To,
    Step,
    EndFor,
    While,
    EndWhile,
    Sub,
    EndSub,
    GoTo,
    Or,
    And,
    Dot, // .
    Comma, // ,
    RightParen, // )
    LeftParen, // (
    RightBracket, // ]
    LeftBracket, // [
    Equal, // =
    NotEqual, // <>
    Plus, // +
    Minus, // -
    Multiply, // *
    Divide, // /
    Colon, // :
    LessThan, // <
    GreaterThan, // >
    LessThanOrEqual, // <=
    GreaterThanOrEqual, // >=
    Identifier,
    NumberLiteral,
    StringLiteral,
    Unrecognized,
};

class Token {
public:
    const TokenTyp type;
    const std::string value;
    const size_t line;
    const size_t column;

    Token(TokenTyp t, std::string v, size_t l, size_t c)
        : type(t), value(std::move(v)), line(l), column(c) {}
};

inline std::string tokenTypeToString(const TokenTyp tokenType) {
    switch (tokenType) {
        case TokenTyp::If: return "If";
        case TokenTyp::Then: return "Then";
        case TokenTyp::Else: return "Else";
        case TokenTyp::ElseIf: return "ElseIf";
        case TokenTyp::EndIf: return "EndIf";
        case TokenTyp::For: return "For";
        case TokenTyp::To: return "To";
        case TokenTyp::Step: return "Step";
        case TokenTyp::EndFor: return "EndFor";
        case TokenTyp::While: return "While";
        case TokenTyp::EndWhile: return "EndWhile";
        case TokenTyp::Sub: return "Sub";
        case TokenTyp::EndSub: return "EndSub";
        case TokenTyp::GoTo: return "GoTo";
        case TokenTyp::Or: return "Or";
        case TokenTyp::And: return "And";
        case TokenTyp::Dot: return "Dot";
        case TokenTyp::Comma: return "Comma";
        case TokenTyp::RightParen: return "RightParen";
        case TokenTyp::LeftParen: return "LeftParen";
        case TokenTyp::RightBracket: return "RightBracket";
        case TokenTyp::LeftBracket: return "LeftBracket";
        case TokenTyp::Equal: return "Equal";
        case TokenTyp::NotEqual: return "NotEqual";
        case TokenTyp::Plus: return "Plus";
        case TokenTyp::Minus: return "Minus";
        case TokenTyp::Multiply: return "Multiply";
        case TokenTyp::Divide: return "Divide";
        case TokenTyp::Colon: return "Colon";
        case TokenTyp::LessThan: return "LessThan";
        case TokenTyp::GreaterThan: return "GreaterThan";
        case TokenTyp::LessThanOrEqual: return "LessThanOrEqual";
        case TokenTyp::GreaterThanOrEqual: return "GreaterThanOrEqual";
        case TokenTyp::Identifier: return "Identifier";
        case TokenTyp::NumberLiteral: return "NumberLiteral";
        case TokenTyp::StringLiteral: return "StringLiteral";
        default: return "Unrecognized";
    }
}