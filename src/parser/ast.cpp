#include "ast.hpp"

void NumberLiteral::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "NumberLiteral: " << this->value << "\n";
}

void StringLiteral::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "StringLiteral: \"" << this->value << "\"\n";
}

void Identifier::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "Identifier: " << this->name << "\n";
}

void ArrayAccess::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "ArrayAccess:\n";
    out << this->getIndent(indent + 1) << "Array:\n";
    this->array->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Index:\n";
    this->index->print(out, indent + 2);
}

void PropertyAccess::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "PropertyAccess:\n";
    out << this->getIndent(indent + 1) << "Object:\n";
    this->object->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Property: " << this->property << "\n";
}

void BinaryExpression::print(std::ostream& out, const int indent) const {
    auto opStr = "";
    switch (this->op) {
        case BinaryOp::Add: opStr = "+"; break;
        case BinaryOp::Subtract: opStr = "-"; break;
        case BinaryOp::Multiply: opStr = "*"; break;
        case BinaryOp::Divide: opStr = "/"; break;
        case BinaryOp::Equal: opStr = "="; break;
        case BinaryOp::NotEqual: opStr = "<>"; break;
        case BinaryOp::LessThan: opStr = "<"; break;
        case BinaryOp::GreaterThan: opStr = ">"; break;
        case BinaryOp::LessThanOrEqual: opStr = "<="; break;
        case BinaryOp::GreaterThanOrEqual: opStr = ">="; break;
        case BinaryOp::And: opStr = "And"; break;
        case BinaryOp::Or: opStr = "Or"; break;
    }

    out << this->getIndent(indent) << "BinaryExpression: " << opStr << "\n";
    out << this->getIndent(indent + 1) << "Left:\n";
    this->left->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Right:\n";
    this->right->print(out, indent + 2);
}

void UnaryExpression::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "UnaryExpression: -\n";
    out << this->getIndent(indent + 1) << "Operand:\n";
    this->operand->print(out, indent + 2);
}

void CallExpression::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "CallExpression:\n";
    out << this->getIndent(indent + 1) << "Callee:\n";
    this->callee->print(out, indent + 2);

    if (!this->arguments.empty()) {
        out << this->getIndent(indent + 1) << "Arguments:\n";
        for (const auto& arg : this->arguments) {
            arg->print(out, indent + 2);
        }
    }
}

void AssignmentStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "AssignmentStatement:\n";
    out << this->getIndent(indent + 1) << "Target:\n";
    this->target->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Value:\n";
    this->value->print(out, indent + 2);
}

void ExpressionStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "ExpressionStatement:\n";
    this->expression->print(out, indent + 1);
}

void IfStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "IfStatement:\n";
    out << this->getIndent(indent + 1) << "Condition:\n";
    this->condition->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Then:\n";
    for (const auto& stmt : this->thenBlock) {
        stmt->print(out, indent + 2);
    }

    for (const auto& [cond, block] : this->elseIfBlocks) {
        out << this->getIndent(indent + 1) << "ElseIf:\n";
        out << this->getIndent(indent + 2) << "Condition:\n";
        cond->print(out, indent + 3);
        out << this->getIndent(indent + 2) << "Block:\n";
        for (const auto& stmt : block) {
            stmt->print(out, indent + 3);
        }
    }

    if (!this->elseBlock.empty()) {
        out << this->getIndent(indent + 1) << "Else:\n";
        for (const auto& stmt : this->elseBlock) {
            stmt->print(out, indent + 2);
        }
    }
}

void WhileStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "WhileStatement:\n";
    out << this->getIndent(indent + 1) << "Condition:\n";
    this->condition->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "Body:\n";
    for (const auto& stmt : this->body) {
        stmt->print(out, indent + 2);
    }
}

void ForStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "ForStatement:\n";
    out << this->getIndent(indent + 1) << "Variable: " << this->variable << "\n";
    out << this->getIndent(indent + 1) << "Start:\n";
    this->start->print(out, indent + 2);
    out << this->getIndent(indent + 1) << "End:\n";
    this->end->print(out, indent + 2);

    if (this->step) {
        out << this->getIndent(indent + 1) << "Step:\n";
        this->step->print(out, indent + 2);
    }

    out << this->getIndent(indent + 1) << "Body:\n";
    for (const auto& stmt : this->body) {
        stmt->print(out, indent + 2);
    }
}

void GotoStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "GotoStatement: " << this->label << "\n";
}

void LabelStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "LabelStatement: " << this->name << "\n";
}

void SubroutineStatement::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "SubroutineStatement: " << this->name << "\n";
    out << this->getIndent(indent + 1) << "Body:\n";
    for (const auto& stmt : this->body) {
        stmt->print(out, indent + 2);
    }
}

void Program::print(std::ostream& out, const int indent) const {
    out << this->getIndent(indent) << "Program:\n";
    for (const auto& stmt : this->statements) {
        stmt->print(out, indent + 1);
    }
}