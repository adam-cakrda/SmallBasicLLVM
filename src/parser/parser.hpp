#pragma once
#include <memory>
#include <vector>
#include "../lexer/token.hpp"
#include "ast.hpp"
#include "../diagnostic.hpp"

class Parser {
public:
    Parser(const std::vector<Token>& toks, DiagnosticReporter& diag)
        : tokens(toks), reporter(diag), pos(0) {}

    std::unique_ptr<Program> parse();

private:
    const std::vector<Token>& tokens;
    DiagnosticReporter& reporter;
    size_t pos;

    const Token& peek(size_t offset = 0) const;
    const Token& current() const;
    Token advance();
    bool isAtEnd() const;
    bool match(TokenTyp type);
    Token consume(TokenTyp type, const std::string& message);

    std::unique_ptr<Statement> makeStatement();
    std::unique_ptr<Statement> makeAssignment();
    std::unique_ptr<Statement> makeIf();
    std::unique_ptr<Statement> makeWhile();
    std::unique_ptr<Statement> makeFor();
    std::unique_ptr<Statement> makeSub();
    std::unique_ptr<Statement> makeGoto();
    std::unique_ptr<Statement> makeLabel();

    std::unique_ptr<Expression> makeExpression();
    std::unique_ptr<Expression> makeAssignmentTarget();
    std::unique_ptr<Expression> makeOr();
    std::unique_ptr<Expression> makeAnd();
    std::unique_ptr<Expression> makeComparison();
    std::unique_ptr<Expression> makeAdditive();
    std::unique_ptr<Expression> makeMultiplicative();
    std::unique_ptr<Expression> makeUnary();
    std::unique_ptr<Expression> makePostfix();
    std::unique_ptr<Expression> makePrimary();

    void skipToNextStatement();
    SourceLocation getLocation() const;
};