#pragma once
#include <memory>
#include <vector>

class ASTNode {
public:
    size_t line;
    size_t column;

    ASTNode(const size_t l, const size_t c) : line(l), column(c) {}
    virtual ~ASTNode() = default;
    virtual void print(std::ostream& out, int indent = 0) const = 0;

protected:
    std::string getIndent(const int indent) const {
        return std::string(indent * 2, ' ');
    }
};

class Expression : public ASTNode {
public:
    Expression(const size_t l, const size_t c) : ASTNode(l, c) {}
};

class NumberLiteral final : public Expression {
public:
    double value;

    NumberLiteral(const double val, const size_t l, const size_t c)
        : Expression(l, c), value(val) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class StringLiteral final : public Expression {
public:
    std::string value;

    StringLiteral(std::string val, const size_t l, const size_t c)
        : Expression(l, c), value(std::move(val)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class Identifier final : public Expression {
public:
    std::string name;

    Identifier(std::string n, const size_t l, const size_t c)
        : Expression(l, c), name(std::move(n)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ArrayAccess final : public Expression {
public:
    std::unique_ptr<Expression> array;
    std::unique_ptr<Expression> index;

    ArrayAccess(std::unique_ptr<Expression> arr, std::unique_ptr<Expression> idx,
                const size_t l, const size_t c)
        : Expression(l, c), array(std::move(arr)), index(std::move(idx)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class PropertyAccess final : public Expression {
public:
    std::unique_ptr<Expression> object;
    std::string property;

    PropertyAccess(std::unique_ptr<Expression> obj, std::string prop,
                   const size_t l, const size_t c)
        : Expression(l, c), object(std::move(obj)), property(std::move(prop)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

enum class BinaryOp {
    Add, Subtract, Multiply, Divide,
    Equal, NotEqual, LessThan, GreaterThan,
    LessThanOrEqual, GreaterThanOrEqual,
    And, Or
};

class BinaryExpression final : public Expression {
public:
    BinaryOp op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;

    BinaryExpression(const BinaryOp operation, std::unique_ptr<Expression> l,
                     std::unique_ptr<Expression> r, const size_t line, const size_t col)
        : Expression(line, col), op(operation), left(std::move(l)), right(std::move(r)) {}

    
    void print(std::ostream& out, int indent = 0) const override;
};

class UnaryExpression final : public Expression {
public:
    std::unique_ptr<Expression> operand;

    UnaryExpression(std::unique_ptr<Expression> opd, const size_t l, const size_t c)
        : Expression(l, c), operand(std::move(opd)) {}

    
    void print(std::ostream& out, int indent = 0) const override;
};

class CallExpression final : public Expression {
public:
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;

    CallExpression(std::unique_ptr<Expression> c,
                   std::vector<std::unique_ptr<Expression>> args,
                   const size_t l, const size_t col)
        : Expression(l, col), callee(std::move(c)), arguments(std::move(args)) {}

    
    void print(std::ostream& out, int indent = 0) const override;
};

class Statement : public ASTNode {
public:
    Statement(const size_t l, const size_t c) : ASTNode(l, c) {}
};

class AssignmentStatement : public Statement {
public:
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> value;

    AssignmentStatement(std::unique_ptr<Expression> tgt, std::unique_ptr<Expression> val,
                        const size_t l, const size_t c)
        : Statement(l, c), target(std::move(tgt)), value(std::move(val)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ExpressionStatement final : public Statement {
public:
    std::unique_ptr<Expression> expression;

    ExpressionStatement(std::unique_ptr<Expression> expr, const size_t l, const size_t c)
        : Statement(l, c), expression(std::move(expr)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class IfStatement final : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> thenBlock;
    std::vector<std::pair<std::unique_ptr<Expression>,
                          std::vector<std::unique_ptr<Statement>>>> elseIfBlocks;
    std::vector<std::unique_ptr<Statement>> elseBlock;

    IfStatement(std::unique_ptr<Expression> cond, const size_t l, const size_t c)
        : Statement(l, c), condition(std::move(cond)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class WhileStatement final : public Statement {
public:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;

    WhileStatement(std::unique_ptr<Expression> cond, const size_t l, const size_t c)
        : Statement(l, c), condition(std::move(cond)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class ForStatement final : public Statement {
public:
    std::string variable;
    std::unique_ptr<Expression> start;
    std::unique_ptr<Expression> end;
    std::unique_ptr<Expression> step;
    std::vector<std::unique_ptr<Statement>> body;

    ForStatement(std::string var, std::unique_ptr<Expression> s,
                 std::unique_ptr<Expression> e, std::unique_ptr<Expression> st,
                 const size_t l, const size_t c)
        : Statement(l, c), variable(std::move(var)), start(std::move(s)),
          end(std::move(e)), step(std::move(st)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class GotoStatement : public Statement {
public:
    std::string label;

    GotoStatement(std::string lbl, const size_t l, const size_t c)
        : Statement(l, c), label(std::move(lbl)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class LabelStatement final : public Statement {
public:
    std::string name;

    LabelStatement(std::string n, const size_t l, const size_t c)
        : Statement(l, c), name(std::move(n)) {}

    void print(std::ostream& out, int indent = 0) const override;
};

class SubroutineStatement final : public Statement {
public:
    std::string name;
    std::vector<std::unique_ptr<Statement>> body;

    SubroutineStatement(std::string n, const size_t l, const size_t c)
        : Statement(l, c), name(std::move(n)) {}

    
    void print(std::ostream& out, int indent = 0) const override;
};

class Program final : public ASTNode {
public:
    std::vector<std::unique_ptr<Statement>> statements;

    Program() : ASTNode(1, 1) {}

    void print(std::ostream& out, int indent = 0) const override;
};