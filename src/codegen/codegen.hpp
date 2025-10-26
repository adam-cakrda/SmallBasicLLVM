#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include "../parser/ast.hpp"
#include "../diagnostic.hpp"
#include "../registry/registry.hpp"

class CodeGenerator {
public:
    explicit CodeGenerator(DiagnosticReporter& diag);
    
    bool generate(const Program& program, const std::string& moduleName);
    void emit(const std::string& filename) const;

private:
    DiagnosticReporter& reporter;
    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;

    llvm::Type* voidTy;
    llvm::Type* i8Ty;
    llvm::Type* i32Ty;
    llvm::Type* i64Ty;
    llvm::Type* doubleTy;
    llvm::PointerType* i8PtrTy;
    llvm::PointerType* valuePtrTy;

    llvm::Function* runtimeInit;
    llvm::Function* runtimeCleanup;
    llvm::Function* valueFromNumber;
    llvm::Function* valueFromString;
    llvm::Function* valueToNumber;
    llvm::Function* valueToString;
    llvm::Function* arrayGet;
    llvm::Function* arraySet;

    llvm::Function* valueAdd;
    llvm::Function* valueSub;
    llvm::Function* valueMul;
    llvm::Function* valueDiv;

    llvm::Function* valueEq;
    llvm::Function* valueNeq;
    llvm::Function* valueLt;
    llvm::Function* valueGt;
    llvm::Function* valueLte;
    llvm::Function* valueGte;

    std::unordered_map<std::string, llvm::Function*> stdFunctions;
    Registry registry;

    std::unordered_map<std::string, llvm::GlobalVariable*> variables;
    std::unordered_map<std::string, llvm::BasicBlock*> labels;
    std::unordered_map<std::string, llvm::Function*> subroutines;
    
    llvm::Function* mainFunction;
    llvm::BasicBlock* currentBlock;

    void generateStatement(Statement& stmt);
    llvm::Value* generateExpression(Expression& expr);
    
    void generateAssignment(AssignmentStatement& stmt);
    void generateExpressionStmt(const ExpressionStatement& stmt);
    void generateIf(IfStatement& stmt);
    void generateWhile(WhileStatement& stmt);
    void generateFor(ForStatement& stmt);
    void generateGoto(GotoStatement& stmt);
    void generateLabel(LabelStatement& stmt);
    void generateSubroutine(SubroutineStatement& stmt);

    llvm::Value* generateBinaryExpr(BinaryExpression& expr);
    llvm::Value* generateUnaryExpr(UnaryExpression& expr);
    llvm::Value* generateCallExpr(const CallExpression& expr);
    llvm::Value* generateIdentifier(Identifier& expr);
    llvm::Value* generateArrayAccess(const ArrayAccess& expr);
    llvm::Value* generatePropertyAccess(const PropertyAccess& expr) const;
    llvm::Value* generateNumberLiteral(NumberLiteral& expr);
    llvm::Value* generateStringLiteral(StringLiteral& expr);

    void declareRuntimeFunctions();
    void createMainFunction();
    llvm::GlobalVariable* createVariable(const std::string& name) const;
    llvm::GlobalVariable* getOrCreateVariable(const std::string& name);
    llvm::BasicBlock* createBlock(const std::string& name) const;
    llvm::Value* createStringConstant(const std::string& str) const;

    llvm::Function* getOrDeclareStdFunction(const std::string& object,
                                            const std::string& method,
                                            const FunctionInfo& info);

    void generateAssignmentTarget(Expression& target, llvm::Value* value);
    void assignToVariable(const std::string& name, llvm::Value* value);
    void assignToArray(const ArrayAccess& access, llvm::Value* value);
    void assignToProperty(const PropertyAccess& access, llvm::Value* value);
};