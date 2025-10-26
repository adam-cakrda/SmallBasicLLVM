#include "parser.hpp"
#include <spdlog/spdlog.h>

std::unique_ptr<Program> Parser::parse() {
    auto program = std::make_unique<Program>();

    while (!this->isAtEnd()) {
        try {
            if (auto stmt = this->makeStatement()) {
                program->statements.push_back(std::move(stmt));
            }
        } catch (const std::exception& e) {
            spdlog::error("Parser error: {}", e.what());
            this->skipToNextStatement();
        }
    }

    return program;
}

const Token& Parser::peek(const size_t offset) const {
    if (this->pos + offset >= this->tokens.size()) {
        return this->tokens.back();
    }
    return this->tokens[this->pos + offset];
}

const Token& Parser::current() const {
    return this->peek(0);
}

Token Parser::advance() {
    if (!this->isAtEnd()) this->pos++;
    return this->peek(-1);
}

bool Parser::isAtEnd() const {
    return this->pos >= this->tokens.size();
}

bool Parser::match(const TokenTyp type) {
    if (this->isAtEnd()) return false;
    if (this->current().type == type) {
        this->advance();
        return true;
    }
    return false;
}

Token Parser::consume(TokenTyp type, const std::string& message) {
    if (this->current().type == type) {
        return this->advance();
    }

    this->reporter.addError(message, this->getLocation(),
        "expected '" + tokenTypeToString(type) + "'");

    return {type, "", this->current().line, this->current().column};
}

SourceLocation Parser::getLocation() const {
    const auto& tok = this->current();
    return SourceLocation(tok.line, tok.column, tok.value.length());
}

void Parser::skipToNextStatement() {
    this->advance();
    while (!this->isAtEnd()) {
        if (this->current().type == TokenTyp::If ||
            this->current().type == TokenTyp::While ||
            this->current().type == TokenTyp::For ||
            this->current().type == TokenTyp::Sub ||
            this->current().type == TokenTyp::GoTo) {
            return;
        }
        this->advance();
    }
}

std::unique_ptr<Statement> Parser::makeStatement() {
    if (this->match(TokenTyp::If)) return this->makeIf();
    if (this->match(TokenTyp::While)) return this->makeWhile();
    if (this->match(TokenTyp::For)) return this->makeFor();
    if (this->match(TokenTyp::Sub)) return this->makeSub();
    if (this->match(TokenTyp::GoTo)) return this->makeGoto();

    if (this->current().type == TokenTyp::Identifier &&
        this->peek(1).type == TokenTyp::Colon) {
        return this->makeLabel();
    }

    return this->makeAssignment();
}

std::unique_ptr<Statement> Parser::makeAssignment() {
    const auto& startToken = this->current();

    if (startToken.type == TokenTyp::Then ||
        startToken.type == TokenTyp::ElseIf ||
        startToken.type == TokenTyp::Else ||
        startToken.type == TokenTyp::EndIf ||
        startToken.type == TokenTyp::EndWhile ||
        startToken.type == TokenTyp::EndFor ||
        startToken.type == TokenTyp::EndSub ||
        startToken.type == TokenTyp::To ||
        startToken.type == TokenTyp::Step) {
        this->reporter.addError(
            "unexpected keyword '" + startToken.value + "'",
            this->getLocation(),
            "expected statement"
        );
        this->advance();
        return nullptr;
    }

    auto expr = this->makeAssignmentTarget();

    if (this->match(TokenTyp::Equal)) {
        auto value = this->makeExpression();
        return std::make_unique<AssignmentStatement>(
            std::move(expr), std::move(value),
            startToken.line, startToken.column
        );
    }

    return std::make_unique<ExpressionStatement>(
        std::move(expr), startToken.line, startToken.column
    );
}

std::unique_ptr<Expression> Parser::makeAssignmentTarget() {
    auto expr = this->makePrimary();

    while (true) {
        if (this->match(TokenTyp::LeftBracket)) {
            const auto& bracketToken = this->peek(-1);
            auto index = this->makeExpression();
            this->consume(TokenTyp::RightBracket, "expected ']'");

            expr = std::make_unique<ArrayAccess>(
                std::move(expr), std::move(index),
                bracketToken.line, bracketToken.column
            );
        }
        else if (this->match(TokenTyp::Dot)) {
            const auto& dotToken = this->peek(-1);
            Token propToken = this->consume(TokenTyp::Identifier, "expected property name");

            expr = std::make_unique<PropertyAccess>(
                std::move(expr), propToken.value,
                dotToken.line, dotToken.column
            );
        }
        else if (this->current().type == TokenTyp::LeftParen) {
            const auto& parenToken = this->current();
            this->advance();
            std::vector<std::unique_ptr<Expression>> arguments;

            if (this->current().type != TokenTyp::RightParen) {
                do {
                    arguments.push_back(this->makeExpression());
                } while (this->match(TokenTyp::Comma));
            }

            this->consume(TokenTyp::RightParen, "expected ')'");

            expr = std::make_unique<CallExpression>(
                std::move(expr), std::move(arguments),
                parenToken.line, parenToken.column
            );
        }
        else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Statement> Parser::makeIf() {
    const auto& ifToken = this->peek(-1);
    auto condition = this->makeExpression();
    this->consume(TokenTyp::Then, "expected 'Then' after if condition");

    auto stmt = std::make_unique<IfStatement>(
        std::move(condition), ifToken.line, ifToken.column
    );

    // Then block
    while (!this->isAtEnd() &&
           this->current().type != TokenTyp::ElseIf &&
           this->current().type != TokenTyp::Else &&
           this->current().type != TokenTyp::EndIf) {
        auto s = this->makeStatement();
        if (s) stmt->thenBlock.push_back(std::move(s));
    }

    // ElseIf blocks
    while (this->match(TokenTyp::ElseIf)) {
        auto cond = this->makeExpression();
        this->consume(TokenTyp::Then, "expected 'Then' after elseif condition");

        std::vector<std::unique_ptr<Statement>> block;
        while (!this->isAtEnd() &&
               this->current().type != TokenTyp::ElseIf &&
               this->current().type != TokenTyp::Else &&
               this->current().type != TokenTyp::EndIf) {
            auto s = this->makeStatement();
            if (s) block.push_back(std::move(s));
        }
        stmt->elseIfBlocks.emplace_back(std::move(cond), std::move(block));
    }

    // Else block
    if (this->match(TokenTyp::Else)) {
        while (!this->isAtEnd() && this->current().type != TokenTyp::EndIf) {
            auto s = this->makeStatement();
            if (s) stmt->elseBlock.push_back(std::move(s));
        }
    }

    this->consume(TokenTyp::EndIf, "expected 'EndIf'");
    return stmt;
}

std::unique_ptr<Statement> Parser::makeWhile() {
    const auto& whileToken = this->peek(-1);
    auto condition = this->makeExpression();

    auto stmt = std::make_unique<WhileStatement>(
        std::move(condition), whileToken.line, whileToken.column
    );

    while (!this->isAtEnd() && this->current().type != TokenTyp::EndWhile) {
        auto s = this->makeStatement();
        if (s) stmt->body.push_back(std::move(s));
    }

    this->consume(TokenTyp::EndWhile, "expected 'EndWhile'");
    return stmt;
}

std::unique_ptr<Statement> Parser::makeFor() {
    const auto& forToken = this->peek(-1);

    const Token varToken = this->consume(TokenTyp::Identifier, "expected variable name");
    this->consume(TokenTyp::Equal, "expected '='");
    auto start = this->makeExpression();
    this->consume(TokenTyp::To, "expected 'To'");
    auto end = this->makeExpression();

    std::unique_ptr<Expression> step = nullptr;
    if (this->match(TokenTyp::Step)) {
        step = this->makeExpression();
    }

    auto stmt = std::make_unique<ForStatement>(
        varToken.value, std::move(start), std::move(end), std::move(step),
        forToken.line, forToken.column
    );

    while (!this->isAtEnd() && this->current().type != TokenTyp::EndFor) {
        auto s = this->makeStatement();
        if (s) stmt->body.push_back(std::move(s));
    }

    this->consume(TokenTyp::EndFor, "expected 'EndFor'");
    return stmt;
}

std::unique_ptr<Statement> Parser::makeSub() {
    const auto& subToken = this->peek(-1);
    const Token nameToken = this->consume(TokenTyp::Identifier, "expected subroutine name");

    auto stmt = std::make_unique<SubroutineStatement>(
        nameToken.value, subToken.line, subToken.column
    );

    while (!this->isAtEnd() && this->current().type != TokenTyp::EndSub) {
        auto s = this->makeStatement();
        if (s) stmt->body.push_back(std::move(s));
    }

    this->consume(TokenTyp::EndSub, "expected 'EndSub'");
    return stmt;
}

std::unique_ptr<Statement> Parser::makeGoto() {
    const auto& gotoToken = this->peek(-1);
    const Token labelToken = this->consume(TokenTyp::Identifier, "expected label");

    return std::make_unique<GotoStatement>(
        labelToken.value, gotoToken.line, gotoToken.column
    );
}

std::unique_ptr<Statement> Parser::makeLabel() {
    const Token labelToken = this->advance();
    this->consume(TokenTyp::Colon, "expected ':'");

    return std::make_unique<LabelStatement>(
        labelToken.value, labelToken.line, labelToken.column
    );
}

std::unique_ptr<Expression> Parser::makeExpression() {
    return this->makeOr();
}

std::unique_ptr<Expression> Parser::makeOr() {
    auto expr = this->makeAnd();

    while (this->match(TokenTyp::Or)) {
        const auto& opToken = this->peek(-1);
        auto right = this->makeAnd();
        expr = std::make_unique<BinaryExpression>(
            BinaryOp::Or, std::move(expr), std::move(right),
            opToken.line, opToken.column
        );
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makeAnd() {
    auto expr = this->makeComparison();

    while (this->match(TokenTyp::And)) {
        const auto& opToken = this->peek(-1);
        auto right = this->makeComparison();
        expr = std::make_unique<BinaryExpression>(
            BinaryOp::And, std::move(expr), std::move(right),
            opToken.line, opToken.column
        );
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makeComparison() {
    auto expr = this->makeAdditive();

    while (true) {
        BinaryOp op;
        bool matched = false;

        if (this->match(TokenTyp::Equal)) {
            op = BinaryOp::Equal;
            matched = true;
        } else if (this->match(TokenTyp::NotEqual)) {
            op = BinaryOp::NotEqual;
            matched = true;
        } else if (this->match(TokenTyp::LessThanOrEqual)) {
            op = BinaryOp::LessThanOrEqual;
            matched = true;
        } else if (this->match(TokenTyp::GreaterThanOrEqual)) {
            op = BinaryOp::GreaterThanOrEqual;
            matched = true;
        } else if (this->match(TokenTyp::LessThan)) {
            op = BinaryOp::LessThan;
            matched = true;
        } else if (this->match(TokenTyp::GreaterThan)) {
            op = BinaryOp::GreaterThan;
            matched = true;
        }

        if (!matched) break;

        const auto& opToken = this->peek(-1);
        auto right = this->makeAdditive();
        expr = std::make_unique<BinaryExpression>(
            op, std::move(expr), std::move(right),
            opToken.line, opToken.column
        );
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makeAdditive() {
    auto expr = this->makeMultiplicative();

    while (true) {
        BinaryOp op;
        bool matched = false;

        if (this->match(TokenTyp::Plus)) {
            op = BinaryOp::Add;
            matched = true;
        } else if (this->match(TokenTyp::Minus)) {
            op = BinaryOp::Subtract;
            matched = true;
        }

        if (!matched) break;

        const auto& opToken = this->peek(-1);
        auto right = this->makeMultiplicative();
        expr = std::make_unique<BinaryExpression>(
            op, std::move(expr), std::move(right),
            opToken.line, opToken.column
        );
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makeMultiplicative() {
    auto expr = this->makeUnary();

    while (true) {
        BinaryOp op;
        bool matched = false;

        if (this->match(TokenTyp::Multiply)) {
            op = BinaryOp::Multiply;
            matched = true;
        } else if (this->match(TokenTyp::Divide)) {
            op = BinaryOp::Divide;
            matched = true;
        }

        if (!matched) break;

        const auto& opToken = this->peek(-1);
        auto right = this->makeUnary();
        expr = std::make_unique<BinaryExpression>(
            op, std::move(expr), std::move(right),
            opToken.line, opToken.column
        );
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makeUnary() {
    if (this->match(TokenTyp::Minus)) {
        const auto& opToken = this->peek(-1);
        auto operand = this->makeUnary();
        return std::make_unique<UnaryExpression>(
            std::move(operand),
            opToken.line, opToken.column
        );
    }

    return this->makePostfix();
}

std::unique_ptr<Expression> Parser::makePostfix() {
    auto expr = this->makePrimary();

    while (true) {
        if (this->match(TokenTyp::LeftBracket)) {
            const auto& bracketToken = this->peek(-1);
            auto index = this->makeExpression();
            this->consume(TokenTyp::RightBracket, "expected ']'");

            expr = std::make_unique<ArrayAccess>(
                std::move(expr), std::move(index),
                bracketToken.line, bracketToken.column
            );
        }
        else if (this->match(TokenTyp::Dot)) {
            const auto& dotToken = this->peek(-1);
            Token propToken = this->consume(TokenTyp::Identifier, "expected property name");

            expr = std::make_unique<PropertyAccess>(
                std::move(expr), propToken.value,
                dotToken.line, dotToken.column
            );
        }
        else if (this->match(TokenTyp::LeftParen)) {
            const auto& parenToken = this->peek(-1);
            std::vector<std::unique_ptr<Expression>> arguments;

            if (this->current().type != TokenTyp::RightParen) {
                do {
                    arguments.push_back(this->makeExpression());
                } while (this->match(TokenTyp::Comma));
            }

            this->consume(TokenTyp::RightParen, "expected ')'");

            expr = std::make_unique<CallExpression>(
                std::move(expr), std::move(arguments),
                parenToken.line, parenToken.column
            );
        }
        else {
            break;
        }
    }

    return expr;
}

std::unique_ptr<Expression> Parser::makePrimary() {
    if (this->match(TokenTyp::NumberLiteral)) {
        const auto& token = this->peek(-1);
        double value = std::stod(token.value);
        return std::make_unique<NumberLiteral>(value, token.line, token.column);
    }

    if (this->match(TokenTyp::StringLiteral)) {
        const auto& token = this->peek(-1);
        return std::make_unique<StringLiteral>(token.value, token.line, token.column);
    }

    if (this->match(TokenTyp::Identifier)) {
        const auto& token = this->peek(-1);
        return std::make_unique<Identifier>(token.value, token.line, token.column);
    }

    if (this->match(TokenTyp::LeftParen)) {
        auto expr = this->makeExpression();
        this->consume(TokenTyp::RightParen, "expected ')'");
        return expr;
    }

    const auto& tok = this->current();
    this->reporter.addError(
        "unexpected token: '" + tok.value + "'",
        this->getLocation(),
        "expected expression"
    );

    if (!this->isAtEnd()) {
        this->advance();
    }

    return std::make_unique<NumberLiteral>(0, tok.line, tok.column);
}