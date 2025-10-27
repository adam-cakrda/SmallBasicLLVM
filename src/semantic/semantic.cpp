#include "semantic.hpp"
#include <ranges>
#include <algorithm>

#define CAST(Type, var, expr) auto var = dynamic_cast<Type*>(expr)

void SemanticAnalyzer::analyze(const Program& program) {
    for (const auto& stmt : program.statements) {
        if (CAST(LabelStatement, labelStmt, stmt.get())) {
            defineLabel(labelStmt->name, labelStmt->line, labelStmt->column);
        } else if (CAST(SubroutineStatement, subStmt, stmt.get())) {
            defineSubroutine(subStmt->name, subStmt->line, subStmt->column);
        }
    }

    for (const auto& stmt : program.statements) {
        analyzeStatement(*stmt);
    }

    verifyAllLabels();
}

void SemanticAnalyzer::analyzeStatement(Statement& stmt) {
    if (CAST(AssignmentStatement, assignStmt, &stmt)) {
        analyzeAssignment(*assignStmt);
    } else if (CAST(ExpressionStatement, exprStmt, &stmt)) {
        analyzeExpression(*exprStmt->expression);
    } else if (CAST(IfStatement, ifStmt, &stmt)) {
        analyzeExpression(*ifStmt->condition);
        for (const auto& s : ifStmt->thenBlock) {
            analyzeStatement(*s);
        }
        for (const auto& [cond, block] : ifStmt->elseIfBlocks) {
            analyzeExpression(*cond);
            for (const auto& s : block) {
                analyzeStatement(*s);
            }
        }
        for (const auto& s : ifStmt->elseBlock) {
            analyzeStatement(*s);
        }
    } else if (CAST(WhileStatement, whileStmt, &stmt)) {
        analyzeExpression(*whileStmt->condition);
        for (const auto& s : whileStmt->body) {
            analyzeStatement(*s);
        }
    } else if (CAST(ForStatement, forStmt, &stmt)) {
        defineVariable(forStmt->variable);
        analyzeExpression(*forStmt->start);
        analyzeExpression(*forStmt->end);
        if (forStmt->step) {
            analyzeExpression(*forStmt->step);
        }
        for (const auto& s : forStmt->body) {
            analyzeStatement(*s);
        }
    } else if (CAST(GotoStatement, gotoStmt, &stmt)) {
        checkGotoTarget(gotoStmt->label, gotoStmt->line, gotoStmt->column);
    } else if (CAST(SubroutineStatement, subStmt, &stmt)) {
        bool wasInSubroutine = inSubroutine;
        inSubroutine = true;
        for (const auto& s : subStmt->body) {
            analyzeStatement(*s);
        }
        inSubroutine = wasInSubroutine;
    }
}

void SemanticAnalyzer::analyzeAssignment(const AssignmentStatement& stmt) {
    if (CAST(PropertyAccess, propAccess, stmt.target.get())) {
        if (CAST(Identifier, objIdent, propAccess->object.get())) {
            if (CAST(Identifier, handlerIdent, stmt.value.get())) {
                const std::string& objectName = objIdent->name;
                const std::string& handlerName = handlerIdent->name;

                if (!registry.hasObject(objectName)) {
                    reporter.addError(
                        "unknown object '" + objectName + "'",
                        SourceLocation(stmt.line, stmt.column, objectName.length()),
                        "this object is not defined in the standard library"
                    );
                }

                std::string handlerNameLower = handlerName;
                std::ranges::transform(handlerNameLower, handlerNameLower.begin(), ::tolower);
                
                if (!subroutines.contains(handlerNameLower)) {
                    reporter.addWarning(
                        "event handler '" + handlerName + "' is not defined",
                        SourceLocation(handlerIdent->line, handlerIdent->column, handlerName.length()),
                        "make sure to define this subroutine before using it as an event handler"
                    );
                }

                return;
            }
        }
    }

    analyzeAssignmentTarget(*stmt.target);
    analyzeExpression(*stmt.value);
}

void SemanticAnalyzer::analyzeAssignmentTarget(Expression& expr) {
    if (CAST(Identifier, ident, &expr)) {
        defineVariable(ident->name);
    } else if (CAST(ArrayAccess, arrAccess, &expr)) {
        analyzeArrayAccess(*arrAccess, true);
    } else if (CAST(PropertyAccess, propAccess, &expr)) {
        analyzePropertyAccess(*propAccess, true);
    } else {
        analyzeExpression(expr);
    }
}

void SemanticAnalyzer::analyzeExpression(Expression& expr) {
    if (CAST(Identifier, ident, &expr)) {
        checkVariable(ident->name, ident->line, ident->column);
    } else if (CAST(BinaryExpression, binExpr, &expr)) {
        analyzeExpression(*binExpr->left);
        analyzeExpression(*binExpr->right);
    } else if (CAST(UnaryExpression, unExpr, &expr)) {
        analyzeExpression(*unExpr->operand);
    } else if (CAST(CallExpression, callExpr, &expr)) {
        analyzeCallExpression(*callExpr);
    } else if (CAST(ArrayAccess, arrAccess, &expr)) {
        analyzeArrayAccess(*arrAccess, false);
    } else if (CAST(PropertyAccess, propAccess, &expr)) {
        analyzePropertyAccess(*propAccess, false);
    }
}

void SemanticAnalyzer::analyzeArrayAccess(const ArrayAccess& expr, const bool isAssignment) {
    if (CAST(Identifier, ident, expr.array.get())) {
        if (isAssignment) {
            defineVariable(ident->name);
        } else {
            checkVariable(ident->name, ident->line, ident->column);
        }
    } else if (CAST(ArrayAccess, nestedAccess, expr.array.get())) {
        analyzeArrayAccess(*nestedAccess, isAssignment);
    } else {
        analyzeExpression(*expr.array);
    }

    analyzeExpression(*expr.index);
}

void SemanticAnalyzer::analyzePropertyAccess(const PropertyAccess& expr, const bool isAssignment) {
    if (CAST(Identifier, objIdent, expr.object.get())) {
        const std::string& objectName = objIdent->name;
        const std::string& propertyName = expr.property;

        if (registry.hasObject(objectName)) {
            if (!registry.hasProperty(objectName, propertyName)) {
                if (!registry.hasFunction(objectName, propertyName)) {
                    reporter.addError(
                        "'" + objectName + "' does not have a property or method '" + propertyName + "'",
                        SourceLocation(expr.line, expr.column, propertyName.length()),
                        "check the spelling or refer to the documentation"
                    );
                }
            } else if (isAssignment) {
                if (auto info = registry.getProperty(objectName, propertyName)) {
                    if (info->readOnly) {
                        reporter.addError(
                            "cannot assign to read-only property '" + objectName + "." + propertyName + "'",
                            SourceLocation(expr.line, expr.column, propertyName.length()),
                            "this property is read-only"
                        );
                    }
                }
            }
        } else {
            checkVariable(objectName, objIdent->line, objIdent->column);
        }
    } else {
        analyzeExpression(*expr.object);
    }
}

void SemanticAnalyzer::analyzeCallExpression(const CallExpression& expr) {
    if (CAST(PropertyAccess, propAccess, expr.callee.get())) {
        if (CAST(Identifier, objIdent, propAccess->object.get())) {
            const std::string& objectName = objIdent->name;
            const std::string& methodName = propAccess->property;

            checkFunction(objectName, methodName, expr.arguments.size(),
                         expr.line, expr.column);
        } else {
            analyzeExpression(*propAccess->object);
        }
    } else if (CAST(Identifier, ident, expr.callee.get())) {
        const std::string& subName = ident->name;
        std::string subNameLower = subName;
        std::ranges::transform(subNameLower, subNameLower.begin(), ::tolower);
        
        if (!subroutines.contains(subNameLower)) {
            reporter.addError(
                "subroutine '" + subName + "' is not defined",
                SourceLocation(ident->line, ident->column, subName.length()),
                "define the subroutine or check the spelling"
            );
        }
    } else {
        analyzeExpression(*expr.callee);
    }

    for (const auto& arg : expr.arguments) {
        analyzeExpression(*arg);
    }
}

void SemanticAnalyzer::checkVariable(const std::string& name, size_t line, size_t col) {
    std::string nameLower = name;
    std::ranges::transform(nameLower, nameLower.begin(), ::tolower);
    
    if (!variables.contains(nameLower)) {
        reporter.addNote(
            "first use of variable '" + name + "'",
            SourceLocation(line, col, name.length()),
            "variables are implicitly initialized to 0 or empty string"
        );
        variables.insert(nameLower);
    }
}

void SemanticAnalyzer::defineVariable(const std::string& name) {
    std::string nameLower = name;
    std::ranges::transform(nameLower, nameLower.begin(), ::tolower);
    variables.insert(nameLower);
}

void SemanticAnalyzer::defineLabel(const std::string& name, const size_t line, const size_t col) {
    if (labels.contains(name)) {
        reporter.addError(
            "label '" + name + "' is already defined",
            SourceLocation(line, col, name.length()),
            "each label must be unique"
        );
    }
    labels.insert(name);
}

void SemanticAnalyzer::defineSubroutine(const std::string& name, const size_t line, const size_t col) {
    std::string nameLower = name;
    std::ranges::transform(nameLower, nameLower.begin(), ::tolower);
    
    if (subroutines.contains(nameLower)) {
        reporter.addError(
            "subroutine '" + name + "' is already defined",
            SourceLocation(line, col, name.length()),
            "each subroutine must be unique"
        );
    }
    subroutines.insert(nameLower);
}

void SemanticAnalyzer::checkGotoTarget(const std::string& label, const size_t line, const size_t col) {
    gotoTargets.insert(label);

    if (inSubroutine) {
        reporter.addWarning(
            "goto statement inside subroutine",
            SourceLocation(line, col, label.length()),
            "using goto inside subroutines can make code harder to understand"
        );
    }
}

void SemanticAnalyzer::verifyAllLabels() const {
    for (const auto& target : gotoTargets) {
        if (!labels.contains(target)) {
            reporter.addError(
                "goto target '" + target + "' is not defined",
                SourceLocation(1, 1, target.length()),
                "define a label with this name or check the spelling"
            );
        }
    }
}

void SemanticAnalyzer::checkFunction(const std::string& object, const std::string& method,
                                     const size_t argCount, const size_t line, const size_t col) const {
    if (!registry.hasObject(object)) {
        reporter.addError(
            "unknown object '" + object + "'",
            SourceLocation(line, col, object.length()),
            "this object is not defined in the standard library"
        );
        return;
    }

    if (!registry.hasFunction(object, method)) {
        reporter.addError(
            "'" + object + "' does not have a method '" + method + "'",
            SourceLocation(line, col, method.length()),
            "check the spelling or refer to the documentation"
        );
        return;
    }

    if (auto funcInfo = registry.getFunction(object, method)) {
        if (argCount != funcInfo->params.size()) {
            reporter.addError(
                "'" + object + "." + method + "' expects " +
                std::to_string(funcInfo->params.size()) + " argument(s), but got " +
                std::to_string(argCount),
                SourceLocation(line, col, method.length()),
                "check the function signature"
            );
        }
    }
}

void SemanticAnalyzer::checkProperty(const std::string& object, const std::string& property,
                                     const size_t line, const size_t col) const {
    if (!registry.hasObject(object)) {
        reporter.addError(
            "unknown object '" + object + "'",
            SourceLocation(line, col, object.length()),
            "this object is not defined in the standard library"
        );
        return;
    }

    if (!registry.hasProperty(object, property)) {
        reporter.addError(
            "'" + object + "' does not have a property '" + property + "'",
            SourceLocation(line, col, property.length()),
            "check the spelling or refer to the documentation"
        );
    }
}