#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include "../parser/ast.hpp"
#include "../diagnostic.hpp"
#include "../registry/registry.hpp"

class SemanticAnalyzer {
public:
    explicit SemanticAnalyzer(DiagnosticReporter& diag)
        : reporter(diag), inSubroutine(false) {}

    void analyze(const Program& program);

private:
    DiagnosticReporter& reporter;
    Registry registry;

    std::set<std::string> variables;
    std::set<std::string> labels;
    std::set<std::string> subroutines;
    std::set<std::string> gotoTargets;

    bool inSubroutine;

    void analyzeStatement(Statement& stmt);
    void analyzeAssignment(const AssignmentStatement& stmt);
    void analyzeAssignmentTarget(Expression& expr);
    void analyzeExpression(Expression& expr);
    void analyzeArrayAccess(const ArrayAccess& expr, bool isAssignment);
    void analyzePropertyAccess(const PropertyAccess& expr, bool isAssignment);
    void analyzeCallExpression(const CallExpression& expr);

    void checkVariable(const std::string& name, size_t line, size_t col);
    void defineVariable(const std::string& name);
    void defineLabel(const std::string& name, size_t line, size_t col);
    void defineSubroutine(const std::string& name, size_t line, size_t col);
    void checkGotoTarget(const std::string& label, size_t line, size_t col);
    void verifyAllLabels() const;
    void checkFunction(const std::string& object, const std::string& method,
                        size_t argCount, size_t line, size_t col) const;
    void checkProperty(const std::string& object, const std::string& property,
                        size_t line, size_t col) const;
};