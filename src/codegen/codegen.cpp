#include "codegen.hpp"
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>

#define CAST(Type, var, expr) auto var = dynamic_cast<Type*>(expr)

CodeGenerator::CodeGenerator(DiagnosticReporter& diag)
    : reporter(diag),
      context(std::make_unique<llvm::LLVMContext>()),
      module(nullptr),
      builder(nullptr),
      mainFunction(nullptr),
      currentBlock(nullptr) {}

bool CodeGenerator::generate(const Program& program, const std::string& moduleName) {
    module = std::make_unique<llvm::Module>(moduleName, *context);
    builder = std::make_unique<llvm::IRBuilder<>>(*context);

    voidTy = llvm::Type::getVoidTy(*context);
    i8Ty = llvm::Type::getInt8Ty(*context);
    i32Ty = llvm::Type::getInt32Ty(*context);
    i64Ty = llvm::Type::getInt64Ty(*context);
    doubleTy = llvm::Type::getDoubleTy(*context);
    i8PtrTy = llvm::PointerType::get(*context, 0);

    valuePtrTy = llvm::PointerType::get(*context, 0);

    declareRuntimeFunctions();

    for (const auto& stmt : program.statements) {
        if (CAST(LabelStatement, labelStmt, stmt.get())) {
            labels[labelStmt->name] = createBlock("label_" + labelStmt->name);
        } else if (CAST(SubroutineStatement, subStmt, stmt.get())) {
            generateSubroutine(*subStmt);
        }
    }

    createMainFunction();

    for (const auto& stmt : program.statements) {
        if (!dynamic_cast<SubroutineStatement*>(stmt.get())) {
            generateStatement(*stmt);
        }
    }

    builder->CreateCall(runtimeCleanup);
    builder->CreateRet(llvm::ConstantInt::get(i32Ty, 0));

    std::string errorStr;
    llvm::raw_string_ostream errorStream(errorStr);
    if (llvm::verifyModule(*module, &errorStream)) {
        reporter.addError("LLVM module verification failed",
            SourceLocation(1, 1, 0), errorStr);
        return false;
    }

    return true;
}

void CodeGenerator::declareRuntimeFunctions() {
    // void runtime_init()
    runtimeInit = llvm::Function::Create(
        llvm::FunctionType::get(voidTy, {}, false),
        llvm::Function::ExternalLinkage,
        "runtime_init",
        module.get()
    );

    // void runtime_cleanup()
    runtimeCleanup = llvm::Function::Create(
        llvm::FunctionType::get(voidTy, {}, false),
        llvm::Function::ExternalLinkage,
        "runtime_cleanup",
        module.get()
    );

    // Value* value_from_number(double)
    valueFromNumber = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {doubleTy}, false),
        llvm::Function::ExternalLinkage,
        "value_from_number",
        module.get()
    );

    // Value* value_from_string(const char*)
    valueFromString = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {i8PtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_from_string",
        module.get()
    );

    // double value_to_number(Value*)
    valueToNumber = llvm::Function::Create(
        llvm::FunctionType::get(doubleTy, {valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_to_number",
        module.get()
    );

    // const char* value_to_string(Value*)
    valueToString = llvm::Function::Create(
        llvm::FunctionType::get(i8PtrTy, {valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_to_string",
        module.get()
    );


    // Value* array_get(Value*, Value*)
    arrayGet = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "array_get",
        module.get()
    );

    // void array_set(Value*, Value*, Value*)
    arraySet = llvm::Function::Create(
        llvm::FunctionType::get(voidTy, {valuePtrTy, valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "array_set",
        module.get()
    );

    // Arithmetic: Value* value_add(Value*, Value*)
    valueAdd = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_add",
        module.get()
    );

    valueSub = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_sub",
        module.get()
    );

    valueMul = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_mul",
        module.get()
    );

    valueDiv = llvm::Function::Create(
        llvm::FunctionType::get(valuePtrTy, {valuePtrTy, valuePtrTy}, false),
        llvm::Function::ExternalLinkage,
        "value_div",
        module.get()
    );

    auto cmpTy = llvm::FunctionType::get(i32Ty, {valuePtrTy, valuePtrTy}, false);
    valueEq = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_eq", module.get());
    valueNeq = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_neq", module.get());
    valueLt = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_lt", module.get());
    valueGt = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_gt", module.get());
    valueLte = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_lte", module.get());
    valueGte = llvm::Function::Create(cmpTy, llvm::Function::ExternalLinkage, "value_gte", module.get());
}

void CodeGenerator::createMainFunction() {
    mainFunction = llvm::Function::Create(
        llvm::FunctionType::get(i32Ty, {}, false),
        llvm::Function::ExternalLinkage,
        "main",
        module.get()
    );

    currentBlock = llvm::BasicBlock::Create(*context, "entry", mainFunction);
    builder->SetInsertPoint(currentBlock);

    builder->CreateCall(runtimeInit);
}

void CodeGenerator::generateStatement(Statement& stmt) {
    if (CAST(AssignmentStatement, assignStmt, &stmt)) {
        generateAssignment(*assignStmt);
    } else if (CAST(ExpressionStatement, exprStmt, &stmt)) {
        generateExpressionStmt(*exprStmt);
    } else if (CAST(IfStatement, ifStmt, &stmt)) {
        generateIf(*ifStmt);
    } else if (CAST(WhileStatement, whileStmt, &stmt)) {
        generateWhile(*whileStmt);
    } else if (CAST(ForStatement, forStmt, &stmt)) {
        generateFor(*forStmt);
    } else if (CAST(GotoStatement, gotoStmt, &stmt)) {
        generateGoto(*gotoStmt);
    } else if (CAST(LabelStatement, labelStmt, &stmt)) {
        generateLabel(*labelStmt);
    }
}

void CodeGenerator::generateAssignment(AssignmentStatement& stmt) {
    llvm::Value* value = generateExpression(*stmt.value);
    generateAssignmentTarget(*stmt.target, value);
}

void CodeGenerator::generateAssignmentTarget(Expression& target, llvm::Value* value) {
    if (CAST(Identifier, ident, &target)) {
        assignToVariable(ident->name, value);
    } else if (CAST(ArrayAccess, arrAccess, &target)) {
        assignToArray(*arrAccess, value);
    } else if (CAST(PropertyAccess, propAccess, &target)) {
        assignToProperty(*propAccess, value);
    }
}

void CodeGenerator::assignToVariable(const std::string& name, llvm::Value* value) {
    llvm::GlobalVariable* var = getOrCreateVariable(name);
    builder->CreateStore(value, var);
}

void CodeGenerator::assignToArray(const ArrayAccess& access, llvm::Value* value) {
    llvm::Value* array = generateExpression(*access.array);
    llvm::Value* index = generateExpression(*access.index);
    builder->CreateCall(arraySet, {array, index, value});
}

void CodeGenerator::assignToProperty(const PropertyAccess& access, llvm::Value* value) {
    if (CAST(Identifier, objIdent, access.object.get())) {
        const std::string& objName = objIdent->name;
        const std::string& propName = access.property;
        if (registry.hasProperty(objName, propName)) {
            std::string objLower = objName;
            std::string propLower = propName;
            std::ranges::transform(objLower, objLower.begin(), ::tolower);
            std::ranges::transform(propLower, propLower.begin(), ::tolower);
            const std::string symbol = objLower + "_" + propLower + "_set";

            llvm::Function* fn = module->getFunction(symbol);
            if (!fn) {
                auto* fty = llvm::FunctionType::get(voidTy, {valuePtrTy}, false);
                fn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage, symbol, module.get());
            }
            builder->CreateCall(fn, {value});
            return;
        }
    }
}

void CodeGenerator::generateExpressionStmt(const ExpressionStatement& stmt) {
    generateExpression(*stmt.expression);
}

void CodeGenerator::generateIf(IfStatement& stmt) {
    llvm::Value* condition = generateExpression(*stmt.condition);
    llvm::Value* condNum = builder->CreateCall(valueToNumber, {condition});
    llvm::Value* cond = builder->CreateFCmpONE(condNum,
        llvm::ConstantFP::get(doubleTy, 0.0));

    llvm::BasicBlock* thenBlock = createBlock("if_then");
    llvm::BasicBlock* elseBlock = createBlock("if_else");
    llvm::BasicBlock* mergeBlock = createBlock("if_merge");

    builder->CreateCondBr(cond, thenBlock, elseBlock);

    // Then block
    builder->SetInsertPoint(thenBlock);
    for (const auto& s : stmt.thenBlock) {
        generateStatement(*s);
    }
    if (!thenBlock->getTerminator()) {
        builder->CreateBr(mergeBlock);
    }

    // ElseIf and Else blocks
    builder->SetInsertPoint(elseBlock);
    llvm::BasicBlock* nextElseBlock = elseBlock;
    
    for (const auto& [elseIfCond, elseIfBlock] : stmt.elseIfBlocks) {
        llvm::Value* elseIfCondVal = generateExpression(*elseIfCond);
        llvm::Value* elseIfCondNum = builder->CreateCall(valueToNumber, {elseIfCondVal});
        llvm::Value* elseIfCmp = builder->CreateFCmpONE(elseIfCondNum,
            llvm::ConstantFP::get(doubleTy, 0.0));

        llvm::BasicBlock* elseIfThen = createBlock("elseif_then");
        llvm::BasicBlock* nextElse = createBlock("elseif_next");

        builder->CreateCondBr(elseIfCmp, elseIfThen, nextElse);

        builder->SetInsertPoint(elseIfThen);
        for (const auto& s : elseIfBlock) {
            generateStatement(*s);
        }
        if (!elseIfThen->getTerminator()) {
            builder->CreateBr(mergeBlock);
        }

        builder->SetInsertPoint(nextElse);
        nextElseBlock = nextElse;
    }

    if (!stmt.elseBlock.empty()) {
        for (const auto& s : stmt.elseBlock) {
            generateStatement(*s);
        }
    }
    if (!nextElseBlock->getTerminator()) {
        builder->CreateBr(mergeBlock);
    }

    builder->SetInsertPoint(mergeBlock);
    currentBlock = mergeBlock;
}

void CodeGenerator::generateWhile(WhileStatement& stmt) {
    llvm::BasicBlock* condBlock = createBlock("while_cond");
    llvm::BasicBlock* bodyBlock = createBlock("while_body");
    llvm::BasicBlock* endBlock = createBlock("while_end");

    builder->CreateBr(condBlock);
    builder->SetInsertPoint(condBlock);

    llvm::Value* condition = generateExpression(*stmt.condition);
    llvm::Value* condNum = builder->CreateCall(valueToNumber, {condition});
    llvm::Value* cond = builder->CreateFCmpONE(condNum,
        llvm::ConstantFP::get(doubleTy, 0.0));

    builder->CreateCondBr(cond, bodyBlock, endBlock);

    builder->SetInsertPoint(bodyBlock);
    for (const auto& s : stmt.body) {
        generateStatement(*s);
    }
    builder->CreateBr(condBlock);

    builder->SetInsertPoint(endBlock);
    currentBlock = endBlock;
}

void CodeGenerator::generateFor(ForStatement& stmt) {
    llvm::Value* startVal = generateExpression(*stmt.start);
    llvm::Value* endVal = generateExpression(*stmt.end);
    
    llvm::Value* stepVal;
    if (stmt.step) {
        stepVal = generateExpression(*stmt.step);
    } else {
        stepVal = builder->CreateCall(valueFromNumber,
            {llvm::ConstantFP::get(doubleTy, 1.0)});
    }

    llvm::GlobalVariable* loopVar = getOrCreateVariable(stmt.variable);
    builder->CreateStore(startVal, loopVar);

    llvm::BasicBlock* condBlock = createBlock("for_cond");
    llvm::BasicBlock* bodyBlock = createBlock("for_body");
    llvm::BasicBlock* incBlock = createBlock("for_inc");
    llvm::BasicBlock* endBlock = createBlock("for_end");

    builder->CreateBr(condBlock);
    builder->SetInsertPoint(condBlock);

    llvm::Value* currentVal = builder->CreateLoad(valuePtrTy, loopVar);

    llvm::Value* currNum = builder->CreateCall(valueToNumber, {currentVal});
    llvm::Value* endNum = builder->CreateCall(valueToNumber, {endVal});
    llvm::Value* condBool = builder->CreateFCmpOLE(currNum, endNum);

    builder->CreateCondBr(condBool, bodyBlock, endBlock);

    builder->SetInsertPoint(bodyBlock);
    for (const auto& s : stmt.body) {
        generateStatement(*s);
    }
    builder->CreateBr(incBlock);

    builder->SetInsertPoint(incBlock);

    llvm::Value* currNum2 = builder->CreateCall(valueToNumber, {currentVal});
    llvm::Value* stepNum = builder->CreateCall(valueToNumber, {stepVal});
    llvm::Value* sum = builder->CreateFAdd(currNum2, stepNum);
    llvm::Value* nextVal = builder->CreateCall(valueFromNumber, {sum});
    builder->CreateStore(nextVal, loopVar);
    builder->CreateBr(condBlock);

    builder->SetInsertPoint(endBlock);
    currentBlock = endBlock;
}

void CodeGenerator::generateGoto(GotoStatement& stmt) {
    if (labels.contains(stmt.label)) {
        builder->CreateBr(labels[stmt.label]);
        

        llvm::BasicBlock* unreachable = createBlock("after_goto");
        builder->SetInsertPoint(unreachable);
        currentBlock = unreachable;
    }
}

void CodeGenerator::generateLabel(LabelStatement& stmt) {
    llvm::BasicBlock* labelBlock = labels[stmt.name];
    
    if (!currentBlock->getTerminator()) {
        builder->CreateBr(labelBlock);
    }
    
    builder->SetInsertPoint(labelBlock);
    currentBlock = labelBlock;
}

void CodeGenerator::generateSubroutine(SubroutineStatement& stmt) {
    llvm::Function* subFunc = llvm::Function::Create(
        llvm::FunctionType::get(voidTy, {}, false),
        llvm::Function::InternalLinkage,
        "sub_" + stmt.name,
        module.get()
    );

    subroutines[stmt.name] = subFunc;

    llvm::BasicBlock* savedBlock = currentBlock;
    auto savedBuilder = builder->saveIP();

    llvm::BasicBlock* subEntry = llvm::BasicBlock::Create(*context, "entry", subFunc);
    builder->SetInsertPoint(subEntry);

    for (const auto& s : stmt.body) {
        generateStatement(*s);
    }

    if (!subEntry->getTerminator()) {
        builder->CreateRetVoid();
    }

    builder->restoreIP(savedBuilder);
    currentBlock = savedBlock;
}

llvm::Value* CodeGenerator::generateExpression(Expression& expr) {
    if (CAST(NumberLiteral, numLit, &expr)) {
        return generateNumberLiteral(*numLit);
    } else if (CAST(StringLiteral, strLit, &expr)) {
        return generateStringLiteral(*strLit);
    } else if (CAST(Identifier, ident, &expr)) {
        return generateIdentifier(*ident);
    } else if (CAST(BinaryExpression, binExpr, &expr)) {
        return generateBinaryExpr(*binExpr);
    } else if (CAST(UnaryExpression, unExpr, &expr)) {
        return generateUnaryExpr(*unExpr);
    } else if (CAST(CallExpression, callExpr, &expr)) {
        return generateCallExpr(*callExpr);
    } else if (CAST(ArrayAccess, arrAccess, &expr)) {
        return generateArrayAccess(*arrAccess);
    } else if (CAST(PropertyAccess, propAccess, &expr)) {
        return generatePropertyAccess(*propAccess);
    }

    return builder->CreateCall(valueFromNumber,
        {llvm::ConstantFP::get(doubleTy, 0.0)});
}

llvm::Value* CodeGenerator::generateNumberLiteral(NumberLiteral& expr) {
    return builder->CreateCall(valueFromNumber,
        {llvm::ConstantFP::get(doubleTy, expr.value)});
}

llvm::Value* CodeGenerator::generateStringLiteral(StringLiteral& expr) {
    return builder->CreateCall(valueFromString,
        {createStringConstant(expr.value)});
}

llvm::Value* CodeGenerator::generateIdentifier(Identifier& expr) {
    llvm::GlobalVariable* var = getOrCreateVariable(expr.name);
    return builder->CreateLoad(valuePtrTy, var);
}

llvm::Value* CodeGenerator::generateBinaryExpr(BinaryExpression& expr) {
    llvm::Value* left = generateExpression(*expr.left);
    llvm::Value* right = generateExpression(*expr.right);

    switch (expr.op) {
        case BinaryOp::Add: {
            return builder->CreateCall(valueAdd, {left, right});
        }
        case BinaryOp::Subtract: {
            return builder->CreateCall(valueSub, {left, right});
        }
        case BinaryOp::Multiply: {
            return builder->CreateCall(valueMul, {left, right});
        }
        case BinaryOp::Divide: {
            return builder->CreateCall(valueDiv, {left, right});
        }
        case BinaryOp::Equal: {
            llvm::Value* cmp = builder->CreateCall(valueEq, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::NotEqual: {
            llvm::Value* cmp = builder->CreateCall(valueNeq, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::LessThan: {
            llvm::Value* cmp = builder->CreateCall(valueLt, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::GreaterThan: {
            llvm::Value* cmp = builder->CreateCall(valueGt, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::LessThanOrEqual: {
            llvm::Value* cmp = builder->CreateCall(valueLte, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::GreaterThanOrEqual: {
            llvm::Value* cmp = builder->CreateCall(valueGte, {left, right});
            llvm::Value* dblVal = builder->CreateSIToFP(cmp, doubleTy);
            return builder->CreateCall(valueFromNumber, {dblVal});
        }
        case BinaryOp::And: {
            llvm::Value* leftNum = builder->CreateCall(valueToNumber, {left});
            llvm::Value* rightNum = builder->CreateCall(valueToNumber, {right});
            llvm::Value* leftBool = builder->CreateFCmpONE(leftNum,
                llvm::ConstantFP::get(doubleTy, 0.0));
            llvm::Value* rightBool = builder->CreateFCmpONE(rightNum,
                llvm::ConstantFP::get(doubleTy, 0.0));
            llvm::Value* result = builder->CreateAnd(leftBool, rightBool);
            llvm::Value* resultNum = builder->CreateUIToFP(result, doubleTy);
            return builder->CreateCall(valueFromNumber, {resultNum});
        }
        case BinaryOp::Or: {
            llvm::Value* leftNum = builder->CreateCall(valueToNumber, {left});
            llvm::Value* rightNum = builder->CreateCall(valueToNumber, {right});
            llvm::Value* leftBool = builder->CreateFCmpONE(leftNum,
                llvm::ConstantFP::get(doubleTy, 0.0));
            llvm::Value* rightBool = builder->CreateFCmpONE(rightNum,
                llvm::ConstantFP::get(doubleTy, 0.0));
            llvm::Value* result = builder->CreateOr(leftBool, rightBool);
            llvm::Value* resultNum = builder->CreateUIToFP(result, doubleTy);
            return builder->CreateCall(valueFromNumber, {resultNum});
        }
    }

    return builder->CreateCall(valueFromNumber,
        {llvm::ConstantFP::get(doubleTy, 0.0)});
}

llvm::Value* CodeGenerator::generateUnaryExpr(UnaryExpression& expr) {
    llvm::Value* operand = generateExpression(*expr.operand);
    llvm::Value* num = builder->CreateCall(valueToNumber, {operand});
    llvm::Value* neg = builder->CreateFSub(llvm::ConstantFP::get(doubleTy, 0.0), num);
    return builder->CreateCall(valueFromNumber, {neg});
}

llvm::Function* CodeGenerator::getOrDeclareStdFunction(const std::string& object,
                                            const std::string& method,
                                            const FunctionInfo& info) {
    const std::string key = object + "." + method;
    if (stdFunctions.contains(key)) return stdFunctions[key];

    std::string objLower = object;
    std::string methodLower = method;
    std::ranges::transform(objLower, objLower.begin(), ::tolower);
    std::ranges::transform(methodLower, methodLower.begin(), ::tolower);
    const std::string symbol = objLower + "_" + methodLower;

    std::vector<llvm::Type*> paramTypes;
    paramTypes.reserve(info.params.size());
    for (const auto p : info.params) {
        (void)p;
        paramTypes.push_back(valuePtrTy);
    }

    llvm::Type* retTy = (info.returnType == ReturnType::Void) ? static_cast<llvm::Type*>(voidTy)
                                                              : static_cast<llvm::Type*>(valuePtrTy);

    auto* fn = llvm::Function::Create(
        llvm::FunctionType::get(retTy, paramTypes, false),
        llvm::Function::ExternalLinkage,
        symbol,
        module.get()
    );

    stdFunctions[key] = fn;
    return fn;
}

llvm::Value* CodeGenerator::generateCallExpr(const CallExpression& expr) {
    if (CAST(PropertyAccess, propAccess, expr.callee.get())) {
        if (CAST(Identifier, objIdent, propAccess->object.get())) {
            const std::string& objName = objIdent->name;
            const std::string& methodName = propAccess->property;

            if (const auto infoOpt = registry.getFunction(objName, methodName)) {
                const FunctionInfo& info = *infoOpt;
                std::vector<llvm::Value*> args;
                args.reserve(expr.arguments.size());

                for (const auto& a : expr.arguments) {
                    args.push_back(generateExpression(*a));
                }
                llvm::Function* fn = getOrDeclareStdFunction(objName, methodName, info);
                if (info.returnType == ReturnType::Void) {
                    builder->CreateCall(fn, args);
                    return builder->CreateCall(valueFromString, {createStringConstant("")});
                }
                return builder->CreateCall(fn, args);
            }
        }
    } else if (CAST(Identifier, ident, expr.callee.get())) {
        if (subroutines.contains(ident->name)) {
            builder->CreateCall(subroutines[ident->name]);
            return builder->CreateCall(valueFromString,
                {createStringConstant("")});
        }
    }

    return builder->CreateCall(valueFromNumber,
        {llvm::ConstantFP::get(doubleTy, 0.0)});
}

llvm::Value* CodeGenerator::generateArrayAccess(const ArrayAccess& expr) {
    llvm::Value* array = generateExpression(*expr.array);
    llvm::Value* index = generateExpression(*expr.index);
    return builder->CreateCall(arrayGet, {array, index});
}

llvm::Value* CodeGenerator::generatePropertyAccess(const PropertyAccess& expr) const {
    if (CAST(Identifier, objIdent, expr.object.get())) {
        const std::string& objName = objIdent->name;
        const std::string& propName = expr.property;
        if (registry.hasProperty(objName, propName)) {
            std::string objLower = objName;
            std::string propLower = propName;
            std::ranges::transform(objLower, objLower.begin(), ::tolower);
            std::ranges::transform(propLower, propLower.begin(), ::tolower);
            const std::string symbol = objLower + "_" + propLower + "_get";
            llvm::Function* fn = module->getFunction(symbol);
            if (!fn) {
                auto* fty = llvm::FunctionType::get(valuePtrTy, {}, false);
                fn = llvm::Function::Create(fty, llvm::Function::ExternalLinkage, symbol, module.get());
            }
            return builder->CreateCall(fn, {});
        }
    }
    
    return builder->CreateCall(valueFromNumber,
        {llvm::ConstantFP::get(doubleTy, 0.0)});
}

llvm::GlobalVariable* CodeGenerator::createVariable(const std::string& name) const {
    auto* gv = new llvm::GlobalVariable(
        *module,
        valuePtrTy,
        false,
        llvm::GlobalValue::PrivateLinkage,
        llvm::ConstantPointerNull::get(valuePtrTy),
        name
    );
    return gv;
}

llvm::GlobalVariable* CodeGenerator::getOrCreateVariable(const std::string& name) {
    if (!variables.contains(name)) {
        variables[name] = createVariable(name);
    }
    return variables[name];
}

llvm::BasicBlock* CodeGenerator::createBlock(const std::string& name) const {
    return llvm::BasicBlock::Create(*context, name, mainFunction);
}

llvm::Value* CodeGenerator::createStringConstant(const std::string& str) const {
    llvm::GlobalVariable* gv = builder->CreateGlobalString(str, "str", 0, module.get());
    llvm::Value* zero32 = llvm::ConstantInt::get(i32Ty, 0);
    llvm::Type* valueTy = gv->getValueType();
    return builder->CreateInBoundsGEP(valueTy, gv, {zero32, zero32});
}

void CodeGenerator::emit(const std::string& filename) const {
    std::error_code ec;
    llvm::raw_fd_ostream out(filename, ec, llvm::sys::fs::OF_None);
    
    if (ec) {
        spdlog::error("Could not open file for writing: {}", ec.message());
        return;
    }
    
    module->print(out, nullptr);
}