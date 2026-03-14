#include "../include/codegen.h"
#include "../include/ast.h"

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/CGSCCPassManager.h"

#include <map>
#include "../include/error.h"
#include "../include/lexer.h"
#include <iostream>

using namespace llvm;

static LLVMContext ctx;
static IRBuilder<> builder(ctx);
static std::unique_ptr<Module> llvmModule;
static std::string sourceFile;

// Variáveis locais (dentro de funções): limpo a cada função
static std::map<std::string, AllocaInst*> localValues;
// Variáveis globais: nunca limpo, acessível de qualquer função
static std::map<std::string, GlobalVariable*> globalValues;
// Arrays locais: nome -> (AllocaInst*, tamanho, tipo elemento)
static std::map<std::string, std::pair<AllocaInst*, Type*>> localArrays;
// Arrays globais: nome -> (GlobalVariable*, tipo elemento)
static std::map<std::string, std::pair<GlobalVariable*, Type*>> globalArrays;

// Structs: nome do tipo -> StructType* LLVM
static std::map<std::string, StructType*> structTypes;
// Structs: nome do tipo -> lista de campos (para calcular índice)
static std::map<std::string, std::vector<StructField>> structFields;
// Variáveis locais do tipo struct: nome -> AllocaInst*
static std::map<std::string, std::pair<AllocaInst*, std::string>> localStructs; // varName -> (alloca, typeName)
// Variáveis globais do tipo struct: nome -> (GlobalVariable*, typeName)
static std::map<std::string, std::pair<GlobalVariable*, std::string>> globalStructs;

static Type* llvmType(DataType t) {
    switch (t) {
        case DataType::Int:    return Type::getInt32Ty(ctx);
        case DataType::Float:  return Type::getFloatTy(ctx);
        case DataType::String: return PointerType::getUnqual(ctx);
        case DataType::Void:   return Type::getVoidTy(ctx);
    }
    return Type::getInt32Ty(ctx);
}

static AllocaInst* createEntryAlloca(Function* fn, const std::string& name, Type* type) {
    IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp.CreateAlloca(type, nullptr, name);
}

static int getFieldIndex(const std::string& typeName, const std::string& fieldName, int line, int col);

static Function* getOrDeclarePrintf() {
    Function* fn = llvmModule->getFunction("printf");
    if (fn) return fn;
    FunctionType* ft = FunctionType::get(
        Type::getInt32Ty(ctx),
        {PointerType::getUnqual(ctx)},
        true
    );
    return Function::Create(ft, Function::ExternalLinkage, "printf", llvmModule.get());
}

static Value* codegenExpr(const ASTNode* node) {
    if (auto* n = dynamic_cast<const IntLitNode*>(node))
        return ConstantInt::get(Type::getInt32Ty(ctx), n->value);

    if (auto* n = dynamic_cast<const FloatLitNode*>(node))
        return ConstantFP::get(Type::getFloatTy(ctx), n->value);

    if (auto* n = dynamic_cast<const StringLitNode*>(node))
        return builder.CreateGlobalStringPtr(n->value);

    // Busca local primeiro, depois global
    if (auto* n = dynamic_cast<const VarNode*>(node)) {
        auto localIt = localValues.find(n->name);
        if (localIt != localValues.end()) {
            AllocaInst* alloca = localIt->second;
            return builder.CreateLoad(alloca->getAllocatedType(), alloca, n->name);
        }
        auto globalIt = globalValues.find(n->name);
        if (globalIt != globalValues.end()) {
            GlobalVariable* gv = globalIt->second;
            return builder.CreateLoad(gv->getValueType(), gv, n->name);
        }
        reportError(sourceFile, n->line, n->col,
                    "The variable '" + n->name + "' was not declared in this scope",
                    getSourceLine(n->line), (int)n->name.size());
        return nullptr;
    }

    if (auto* n = dynamic_cast<const UnaryOpNode*>(node)) {
        Value* val = codegenExpr(n->operand.get());
        // Converte para i1: val != 0
        Value* asBool = val->getType()->isFloatTy()
            ? builder.CreateFCmpONE(val, ConstantFP::get(val->getType(), 0.0))
            : builder.CreateICmpNE(val, ConstantInt::get(val->getType(), 0));
        // !: inverte o i1 e estende para i32
        Value* notVal = builder.CreateNot(asBool);
        return builder.CreateZExt(notVal, Type::getInt32Ty(ctx));
    }

    if (auto* n = dynamic_cast<const BinaryOpNode*>(node)) {
        // Curto-circuito para && e ||
        if (n->op == "&&" || n->op == "||") {
            Function* fn = builder.GetInsertBlock()->getParent();
            Value* lVal = codegenExpr(n->left.get());
            Value* lBool = lVal->getType()->isFloatTy()
                ? builder.CreateFCmpONE(lVal, ConstantFP::get(lVal->getType(), 0.0))
                : builder.CreateICmpNE(lVal, ConstantInt::get(lVal->getType(), 0));

            BasicBlock* evalRhsBB = BasicBlock::Create(ctx, "logic.rhs", fn);
            BasicBlock* mergeBB   = BasicBlock::Create(ctx, "logic.merge", fn);
            BasicBlock* entryBB   = builder.GetInsertBlock();

            if (n->op == "&&") {
                // se lhs é falso, pula direto pro merge com 0
                builder.CreateCondBr(lBool, evalRhsBB, mergeBB);
            } else {
                // se lhs é verdadeiro, pula direto pro merge com 1
                builder.CreateCondBr(lBool, mergeBB, evalRhsBB);
            }

            builder.SetInsertPoint(evalRhsBB);
            Value* rVal = codegenExpr(n->right.get());
            Value* rBool = rVal->getType()->isFloatTy()
                ? builder.CreateFCmpONE(rVal, ConstantFP::get(rVal->getType(), 0.0))
                : builder.CreateICmpNE(rVal, ConstantInt::get(rVal->getType(), 0));
            Value* rExt = builder.CreateZExt(rBool, Type::getInt32Ty(ctx));
            BasicBlock* rhsDoneBB = builder.GetInsertBlock();
            builder.CreateBr(mergeBB);

            builder.SetInsertPoint(mergeBB);
            PHINode* phi = builder.CreatePHI(Type::getInt32Ty(ctx), 2, "logic.result");
            int shortVal = (n->op == "&&") ? 0 : 1;
            phi->addIncoming(ConstantInt::get(Type::getInt32Ty(ctx), shortVal), entryBB);
            phi->addIncoming(rExt, rhsDoneBB);
            return phi;
        }

        Value* l = codegenExpr(n->left.get());
        Value* r = codegenExpr(n->right.get());

        bool lIsFloat  = l->getType()->isFloatTy();
        bool rIsFloat  = r->getType()->isFloatTy();
        bool lIsInt    = l->getType()->isIntegerTy();
        bool rIsInt    = r->getType()->isIntegerTy();
        bool lIsString = l->getType()->isPointerTy();
        bool rIsString = r->getType()->isPointerTy();

        // Verificação de tipos em operações aritméticas
        if (n->op == "+" || n->op == "-" || n->op == "*" ||
            n->op == "/" || n->op == "%") {

            if (lIsString || rIsString)
                reportError(sourceFile, n->line, n->col,
                    "operator '" + n->op + "' cannot be applied to string values",
                    getSourceLine(n->line), (int)n->op.size());

            if (lIsFloat && rIsInt)
                reportError(sourceFile, n->line, n->col,
                    "type mismatch: cannot apply '" + n->op + "' to 'float' and 'int'",
                    getSourceLine(n->line), (int)n->op.size());

            if (lIsInt && rIsFloat)
                reportError(sourceFile, n->line, n->col,
                    "type mismatch: cannot apply '" + n->op + "' to 'int' and 'float'",
                    getSourceLine(n->line), (int)n->op.size());
        }

        if (n->op == "%" && (lIsFloat || rIsFloat))
            reportError(sourceFile, n->line, n->col,
                "operator '%' cannot be applied to float values",
                getSourceLine(n->line), (int)n->op.size());

        bool isFloat = lIsFloat || rIsFloat;

        if (n->op == "+") return isFloat ? builder.CreateFAdd(l, r) : builder.CreateAdd(l, r);
        if (n->op == "-") return isFloat ? builder.CreateFSub(l, r) : builder.CreateSub(l, r);
        if (n->op == "*") return isFloat ? builder.CreateFMul(l, r) : builder.CreateMul(l, r);
        if (n->op == "/") return isFloat ? builder.CreateFDiv(l, r) : builder.CreateSDiv(l, r);
        if (n->op == "%") return isFloat ? builder.CreateFRem(l, r) : builder.CreateSRem(l, r);

        Value* cmp = nullptr;
        if (isFloat) {
            if (n->op == "==") cmp = builder.CreateFCmpOEQ(l, r);
            if (n->op == "!=") cmp = builder.CreateFCmpONE(l, r);
            if (n->op == "<")  cmp = builder.CreateFCmpOLT(l, r);
            if (n->op == ">")  cmp = builder.CreateFCmpOGT(l, r);
            if (n->op == "<=") cmp = builder.CreateFCmpOLE(l, r);
            if (n->op == ">=") cmp = builder.CreateFCmpOGE(l, r);
        } else {
            if (n->op == "==") cmp = builder.CreateICmpEQ(l, r);
            if (n->op == "!=") cmp = builder.CreateICmpNE(l, r);
            if (n->op == "<")  cmp = builder.CreateICmpSLT(l, r);
            if (n->op == ">")  cmp = builder.CreateICmpSGT(l, r);
            if (n->op == "<=") cmp = builder.CreateICmpSLE(l, r);
            if (n->op == ">=") cmp = builder.CreateICmpSGE(l, r);
        }
        if (cmp)
            return builder.CreateZExt(cmp, Type::getInt32Ty(ctx));

        reportError(sourceFile, 0, 0, "The operator '" + n->op + "' is not supported", "");
        return nullptr;
    }

    if (auto* n = dynamic_cast<const CallNode*>(node)) {
        Function* fn = llvmModule->getFunction(n->name);
        if (!fn)
            reportError(sourceFile, n->line, n->col,
                        "function '" + n->name + "' was not declared",
                        getSourceLine(n->line), (int)n->name.size());
        std::vector<Value*> args;
        for (auto& arg : n->args)
            args.push_back(codegenExpr(arg.get()));
        return builder.CreateCall(fn, args);
    }

    if (auto* n = dynamic_cast<const ArrayAccessNode*>(node)) {
        Value* idx = codegenExpr(n->index.get());
        // local primeiro
        auto lit = localArrays.find(n->name);
        if (lit != localArrays.end()) {
            Value* gep = builder.CreateGEP(lit->second.second,
                                           lit->second.first,
                                           {ConstantInt::get(Type::getInt32Ty(ctx), 0), idx});
            return builder.CreateLoad(lit->second.second->getArrayElementType(), gep, n->name);
        }
        auto git = globalArrays.find(n->name);
        if (git != globalArrays.end()) {
            Value* gep = builder.CreateGEP(git->second.second,
                                           git->second.first,
                                           {ConstantInt::get(Type::getInt32Ty(ctx), 0), idx});
            return builder.CreateLoad(git->second.second->getArrayElementType(), gep, n->name);
        }
        reportError(sourceFile, n->line, n->col,
                    "array '" + n->name + "' was not declared in this scope",
                    getSourceLine(n->line), (int)n->name.size());
        return nullptr;
    }

    if (auto* n = dynamic_cast<const FieldAccessNode*>(node)) {
        // local struct first
        auto lit = localStructs.find(n->varName);
        if (lit != localStructs.end()) {
            const std::string& typeName = lit->second.second;
            int idx = getFieldIndex(typeName, n->fieldName, n->line, n->col);
            StructType* st = structTypes[typeName];
            Value* gep = builder.CreateStructGEP(st, lit->second.first, idx, n->fieldName);
            return builder.CreateLoad(st->getElementType(idx), gep, n->fieldName);
        }
        auto git = globalStructs.find(n->varName);
        if (git != globalStructs.end()) {
            const std::string& typeName = git->second.second;
            int idx = getFieldIndex(typeName, n->fieldName, n->line, n->col);
            StructType* st = structTypes[typeName];
            Value* gep = builder.CreateStructGEP(st, git->second.first, idx, n->fieldName);
            return builder.CreateLoad(st->getElementType(idx), gep, n->fieldName);
        }
        reportError(sourceFile, n->line, n->col,
                    "variable '" + n->varName + "' was not declared in this scope",
                    getSourceLine(n->line), (int)n->varName.size());
        return nullptr;
    }

    if (auto* n = dynamic_cast<const MethodCallNode*>(node)) {
        // Resolve o ponteiro para o struct (local ou global)
        Value* selfPtr = nullptr;
        std::string typeName;
        auto lit = localStructs.find(n->varName);
        if (lit != localStructs.end()) {
            selfPtr  = lit->second.first;
            typeName = lit->second.second;
        } else {
            auto git = globalStructs.find(n->varName);
            if (git != globalStructs.end()) {
                selfPtr  = git->second.first;
                typeName = git->second.second;
            }
        }
        if (!selfPtr)
            reportError(sourceFile, n->line, n->col,
                "variable '" + n->varName + "' was not declared in this scope",
                getSourceLine(n->line), (int)n->varName.size());

        std::string funcName = typeName + "__" + n->methodName;
        Function* fn = llvmModule->getFunction(funcName);
        if (!fn)
            reportError(sourceFile, n->line, n->col,
                "struct '" + typeName + "' has no method '" + n->methodName + "'",
                getSourceLine(n->line), (int)n->methodName.size());

        std::vector<Value*> args;
        args.push_back(selfPtr); // self
        for (auto& arg : n->args)
            args.push_back(codegenExpr(arg.get()));
        return builder.CreateCall(fn, args);
    }

    reportError(sourceFile, 0, 0, "Internal error: unknown node type in codegen", "");
    return nullptr;
}

static int getFieldIndex(const std::string& typeName, const std::string& fieldName, int line, int col) {
    auto it = structFields.find(typeName);
    if (it == structFields.end())
        reportError(sourceFile, line, col, "struct type '" + typeName + "' was not declared", getSourceLine(line), (int)typeName.size());
    auto& fields = it->second;
    for (int i = 0; i < (int)fields.size(); i++)
        if (fields[i].name == fieldName) return i;
    reportError(sourceFile, line, col, "struct '" + typeName + "' has no field '" + fieldName + "'", getSourceLine(line), (int)fieldName.size());
    return -1;
}

static void codegenStmt(const ASTNode* node, Function* fn) {
    // ── Declaração de variável de struct local: Point p; ─────────────────
    if (auto* n = dynamic_cast<const StructVarDeclNode*>(node)) {
        auto it = structTypes.find(n->typeName);
        if (it == structTypes.end())
            reportError(sourceFile, n->line, n->col,
                        "struct type '" + n->typeName + "' was not declared",
                        getSourceLine(n->line), (int)n->typeName.size());
        StructType* st = it->second;
        AllocaInst* alloca = createEntryAlloca(fn, n->varName, st);
        localStructs[n->varName] = {alloca, n->typeName};
        return;
    }

    // ── Atribuição de campo: p.x = expr; ─────────────────────────────────
    if (auto* n = dynamic_cast<const FieldAssignNode*>(node)) {
        Value* val = codegenExpr(n->value.get());
        auto lit = localStructs.find(n->varName);
        if (lit != localStructs.end()) {
            const std::string& typeName = lit->second.second;
            int idx = getFieldIndex(typeName, n->fieldName, n->line, n->col);
            StructType* st = structTypes[typeName];
            Value* gep = builder.CreateStructGEP(st, lit->second.first, idx, n->fieldName);
            builder.CreateStore(val, gep);
            return;
        }
        auto git = globalStructs.find(n->varName);
        if (git != globalStructs.end()) {
            const std::string& typeName = git->second.second;
            int idx = getFieldIndex(typeName, n->fieldName, n->line, n->col);
            StructType* st = structTypes[typeName];
            Value* gep = builder.CreateStructGEP(st, git->second.first, idx, n->fieldName);
            builder.CreateStore(val, gep);
            return;
        }
        reportError(sourceFile, n->line, n->col,
                    "variable '" + n->varName + "' was not declared in this scope",
                    getSourceLine(n->line), (int)n->varName.size());
        return;
    }

    if (auto* n = dynamic_cast<const ArrayDeclNode*>(node)) {
        Type* elemType = llvmType(n->type);
        ArrayType* arrType = ArrayType::get(elemType, n->size);
        AllocaInst* alloca = createEntryAlloca(fn, n->name, arrType);
        localArrays[n->name] = {alloca, arrType};
        for (int i = 0; i < (int)n->init.size(); i++) {
            Value* val = codegenExpr(n->init[i].get());
            Value* gep = builder.CreateGEP(arrType, alloca,
                {ConstantInt::get(Type::getInt32Ty(ctx), 0),
                 ConstantInt::get(Type::getInt32Ty(ctx), i)});
            builder.CreateStore(val, gep);
        }
        return;
    }

    if (auto* n = dynamic_cast<const ArrayAssignNode*>(node)) {
        Value* idx = codegenExpr(n->index.get());
        Value* val = codegenExpr(n->value.get());
        auto lit = localArrays.find(n->name);
        if (lit != localArrays.end()) {
            Value* gep = builder.CreateGEP(lit->second.second, lit->second.first,
                {ConstantInt::get(Type::getInt32Ty(ctx), 0), idx});
            builder.CreateStore(val, gep);
            return;
        }
        auto git = globalArrays.find(n->name);
        if (git != globalArrays.end()) {
            Value* gep = builder.CreateGEP(git->second.second, git->second.first,
                {ConstantInt::get(Type::getInt32Ty(ctx), 0), idx});
            builder.CreateStore(val, gep);
            return;
        }
        reportError(sourceFile, n->line, n->col,
                    "array '" + n->name + "' was not declared in this scope",
                    getSourceLine(n->line), (int)n->name.size());
        return;
    }

    if (auto* n = dynamic_cast<const VarAssignNode*>(node)) {
        // Resolve o alloca/global da variável (local tem prioridade)
        AllocaInst*     localAlloca = nullptr;
        GlobalVariable* globalVar   = nullptr;
        auto localIt  = localValues.find(n->name);
        auto globalIt = globalValues.find(n->name);
        if (localIt != localValues.end())
            localAlloca = localIt->second;
        else if (globalIt != globalValues.end())
            globalVar = globalIt->second;
        else
            reportError(sourceFile, n->line, n->col,
                        "The variable '" + n->name + "' has not been declared (use 'int', 'float', or 'string' to declare it)",
                        getSourceLine(n->line), (int)n->name.size());

        Type* varType = localAlloca ? localAlloca->getAllocatedType()
                                    : globalVar->getValueType();
        bool isFloat  = varType->isFloatTy();

        auto loadCurrent = [&]() -> Value* {
            return localAlloca ? builder.CreateLoad(varType, localAlloca, n->name)
                               : builder.CreateLoad(varType, globalVar,   n->name);
        };
        auto store = [&](Value* v) {
            if (localAlloca) builder.CreateStore(v, localAlloca);
            else             builder.CreateStore(v, globalVar);
        };

        Value* newVal = nullptr;
        if (n->op == "=") {
            newVal = codegenExpr(n->expr.get());
        } else if (n->op == "++") {
            Value* cur = loadCurrent();
            newVal = isFloat ? builder.CreateFAdd(cur, ConstantFP::get(varType, 1.0))
                             : builder.CreateAdd(cur,  ConstantInt::get(varType, 1));
        } else if (n->op == "--") {
            Value* cur = loadCurrent();
            newVal = isFloat ? builder.CreateFSub(cur, ConstantFP::get(varType, 1.0))
                             : builder.CreateSub(cur,  ConstantInt::get(varType, 1));
        } else {
            Value* cur = loadCurrent();
            Value* rhs = codegenExpr(n->expr.get());
            if (n->op == "+=") newVal = isFloat ? builder.CreateFAdd(cur, rhs) : builder.CreateAdd(cur, rhs);
            if (n->op == "-=") newVal = isFloat ? builder.CreateFSub(cur, rhs) : builder.CreateSub(cur, rhs);
            if (n->op == "*=") newVal = isFloat ? builder.CreateFMul(cur, rhs) : builder.CreateMul(cur, rhs);
            if (n->op == "/=") newVal = isFloat ? builder.CreateFDiv(cur, rhs) : builder.CreateSDiv(cur, rhs);
        }
        store(newVal);
        return;
    }

    if (auto* n = dynamic_cast<const VarDeclNode*>(node)) {
        Type* type = llvmType(n->type);
        AllocaInst* alloca = createEntryAlloca(fn, n->name, type);
        localValues[n->name] = alloca;
        if (n->init) {
            // Verifica mismatch entre tipo declarado e tipo do literal inicial
            bool declaredFloat = (n->type == DataType::Float);
            bool declaredInt   = (n->type == DataType::Int);
            bool initIsInt     = dynamic_cast<const IntLitNode*>(n->init.get()) != nullptr;
            bool initIsFloat   = dynamic_cast<const FloatLitNode*>(n->init.get()) != nullptr;

            if (declaredFloat && initIsInt)
                reportError(sourceFile, n->line, n->col,
                    "type mismatch: variable '" + n->name + "' is declared as 'float' "
                    "but assigned an 'int' literal — use '" + n->name + " = " + 
                    std::to_string(dynamic_cast<const IntLitNode*>(n->init.get())->value) + ".0' instead",
                    getSourceLine(n->line), (int)n->name.size());

            if (declaredInt && initIsFloat)
                reportError(sourceFile, n->line, n->col,
                    "type mismatch: variable '" + n->name + "' is declared as 'int' "
                    "but assigned a 'float' literal — use 'float " + n->name + "' instead",
                    getSourceLine(n->line), (int)n->name.size());

            Value* val = codegenExpr(n->init.get());
            builder.CreateStore(val, alloca);
        }
        return;
    }

    if (auto* n = dynamic_cast<const ReturnNode*>(node)) {
        if (!n->expr) {
            builder.CreateRetVoid();
        } else {
            Value* val = codegenExpr(n->expr.get());
            builder.CreateRet(val);
        }
        return;
    }

if (auto* n = dynamic_cast<const PrintNode*>(node)) {
    Value* val = codegenExpr(n->expr.get());
    Function* printfFn = getOrDeclarePrintf();
    
    Value* formatStr;
    std::vector<Value*> args;

    if (val->getType()->isFloatTy()) {
        // CORREÇÃO: Converte float (32-bit) para double (64-bit)
        Value* valDouble = builder.CreateFPExt(val, Type::getDoubleTy(ctx));
        formatStr = builder.CreateGlobalStringPtr("%f\n");
        args = { formatStr, valDouble };
    } else if (val->getType()->isIntegerTy()) {
        formatStr = builder.CreateGlobalStringPtr("%d\n");
        args = { formatStr, val };
    } else {
        formatStr = builder.CreateGlobalStringPtr("%s\n");
        args = { formatStr, val };
    }

    builder.CreateCall(printfFn, args);
    return;
}
    if (auto* n = dynamic_cast<const IfNode*>(node)) {
        Value* cond = codegenExpr(n->condition.get());
        cond = builder.CreateICmpNE(cond, ConstantInt::get(Type::getInt32Ty(ctx), 0));

        BasicBlock* thenBB  = BasicBlock::Create(ctx, "then", fn);
        BasicBlock* elseBB  = BasicBlock::Create(ctx, "else", fn);
        BasicBlock* mergeBB = BasicBlock::Create(ctx, "merge", fn);

        builder.CreateCondBr(cond, thenBB, elseBB);

        builder.SetInsertPoint(thenBB);
        for (auto& stmt : n->thenBlock)
            codegenStmt(stmt.get(), fn);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(mergeBB);

        builder.SetInsertPoint(elseBB);
        for (auto& stmt : n->elseBlock)
            codegenStmt(stmt.get(), fn);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(mergeBB);

        builder.SetInsertPoint(mergeBB);
        return;
    }

    if (auto* n = dynamic_cast<const WhileNode*>(node)) {
        BasicBlock* condBB  = BasicBlock::Create(ctx, "while.cond", fn);
        BasicBlock* bodyBB  = BasicBlock::Create(ctx, "while.body", fn);
        BasicBlock* afterBB = BasicBlock::Create(ctx, "while.after", fn);

        builder.CreateBr(condBB);

        builder.SetInsertPoint(condBB);
        Value* cond = codegenExpr(n->condition.get());
        cond = builder.CreateICmpNE(cond, ConstantInt::get(Type::getInt32Ty(ctx), 0));
        builder.CreateCondBr(cond, bodyBB, afterBB);

        builder.SetInsertPoint(bodyBB);
        for (auto& stmt : n->body)
            codegenStmt(stmt.get(), fn);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(condBB);

        builder.SetInsertPoint(afterBB);
        return;
    }

    if (auto* n = dynamic_cast<const ForNode*>(node)) {
        // init (pode declarar variável nova no escopo local)
        if (n->init) codegenStmt(n->init.get(), fn);

        BasicBlock* condBB  = BasicBlock::Create(ctx, "for.cond", fn);
        BasicBlock* bodyBB  = BasicBlock::Create(ctx, "for.body", fn);
        BasicBlock* stepBB  = BasicBlock::Create(ctx, "for.step", fn);
        BasicBlock* afterBB = BasicBlock::Create(ctx, "for.after", fn);

        builder.CreateBr(condBB);

        builder.SetInsertPoint(condBB);
        Value* cond = codegenExpr(n->condition.get());
        cond = builder.CreateICmpNE(cond, ConstantInt::get(Type::getInt32Ty(ctx), 0));
        builder.CreateCondBr(cond, bodyBB, afterBB);

        builder.SetInsertPoint(bodyBB);
        for (auto& stmt : n->body)
            codegenStmt(stmt.get(), fn);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(stepBB);

        builder.SetInsertPoint(stepBB);
        if (n->step) codegenStmt(n->step.get(), fn);
        builder.CreateBr(condBB);

        builder.SetInsertPoint(afterBB);
        return;
    }

    codegenExpr(node);
}

static void codegenFunction(const FunctionNode* fn) {
    std::vector<Type*> paramTypes;
    for (auto& p : fn->params)
        paramTypes.push_back(llvmType(p.type));

    FunctionType* ft = FunctionType::get(llvmType(fn->returnType), paramTypes, false);
    // Reutiliza a declaração da primeira passagem
    Function* func = llvmModule->getFunction(fn->name);
    if (!func)
        func = Function::Create(ft, Function::ExternalLinkage, fn->name, llvmModule.get());

    size_t i = 0;
    for (auto& arg : func->args())
        arg.setName(fn->params[i++].name);

    BasicBlock* bb = BasicBlock::Create(ctx, "entry", func);
    builder.SetInsertPoint(bb);

    // Limpa só locais — globais permanecem
    localValues.clear();
    localArrays.clear();
    localStructs.clear();

    for (auto& arg : func->args()) {
        AllocaInst* alloca = createEntryAlloca(func, std::string(arg.getName()), arg.getType());
        builder.CreateStore(&arg, alloca);
        localValues[std::string(arg.getName())] = alloca;
    }

    for (auto& stmt : fn->body)
        codegenStmt(stmt.get(), func);

    if (!builder.GetInsertBlock()->getTerminator()) {
        if (fn->returnType == DataType::Void)
            builder.CreateRetVoid();
        else
            builder.CreateRet(ConstantInt::get(Type::getInt32Ty(ctx), 0));
    }

    verifyFunction(*func);
}

static void codegenGlobalVar(const VarDeclNode* node) {
    Type* type = llvmType(node->type);
    Constant* init = nullptr;

    if (node->init) {
        bool declaredFloat = (node->type == DataType::Float);
        bool declaredInt   = (node->type == DataType::Int);
        bool initIsInt     = dynamic_cast<const IntLitNode*>(node->init.get()) != nullptr;
        bool initIsFloat   = dynamic_cast<const FloatLitNode*>(node->init.get()) != nullptr;

        if (declaredFloat && initIsInt)
            reportError(sourceFile, node->line, node->col,
                "type mismatch: variable '" + node->name + "' is declared as 'float' "
                "but assigned an 'int' literal — use '" + node->name + " = " +
                std::to_string(dynamic_cast<const IntLitNode*>(node->init.get())->value) + ".0' instead",
                getSourceLine(node->line), (int)node->name.size());

        if (declaredInt && initIsFloat)
            reportError(sourceFile, node->line, node->col,
                "type mismatch: variable '" + node->name + "' is declared as 'int' "
                "but assigned a 'float' literal — use 'float " + node->name + "' instead",
                getSourceLine(node->line), (int)node->name.size());

        if (auto* n = dynamic_cast<const IntLitNode*>(node->init.get()))
            init = ConstantInt::get(type, n->value);
        else if (auto* n = dynamic_cast<const FloatLitNode*>(node->init.get()))
            init = ConstantFP::get(type, n->value);
        else if (auto* n = dynamic_cast<const StringLitNode*>(node->init.get())) {
            // String global: cria uma constante de string e armazena o ponteiro
            Constant* strConst = ConstantDataArray::getString(ctx, n->value, true);
            auto* strGV = new GlobalVariable(*llvmModule, strConst->getType(), true,
                                              GlobalValue::PrivateLinkage, strConst,
                                              node->name + ".str");
            strGV->setUnnamedAddr(GlobalValue::UnnamedAddr::Global);
            init = ConstantExpr::getPointerCast(strGV, PointerType::getUnqual(ctx));
        }
        else
            reportError(sourceFile, node->line, node->col,
                       "global variable '" + node->name + "' can only be initialized with a literal — expressions are not allowed in global scope",
                        getSourceLine(node->line), (int)node->name.size());
    } else {
        init = Constant::getNullValue(type);
    }

    auto* gv = new GlobalVariable(*llvmModule, type, false,
                                   GlobalValue::ExternalLinkage, init, node->name);
    globalValues[node->name] = gv;
}


static void codegenGlobalArray(const ArrayDeclNode* node) {
    Type* elemType = llvmType(node->type);
    ArrayType* arrType = ArrayType::get(elemType, node->size);
    Constant* init = nullptr;
    if (!node->init.empty()) {
        std::vector<Constant*> vals;
        for (auto& e : node->init) {
            if (auto* n = dynamic_cast<const IntLitNode*>(e.get()))
                vals.push_back(ConstantInt::get(elemType, n->value));
            else if (auto* n = dynamic_cast<const FloatLitNode*>(e.get()))
                vals.push_back(ConstantFP::get(elemType, n->value));
        }
        init = ConstantArray::get(arrType, vals);
    } else {
        init = ConstantAggregateZero::get(arrType);
    }
    auto* gv = new GlobalVariable(*llvmModule, arrType, false,
                                   GlobalValue::ExternalLinkage, init, node->name);
    globalArrays[node->name] = {gv, arrType};
}

void codegenProgram(const ProgramNode& program, const std::string& outputFile,
                    const std::string& filename, int optLevel) {
    sourceFile = filename;
    llvmModule = std::make_unique<Module>("nova_module", ctx);

    // Primeira passagem: registra structs e declara funções
    for (auto& decl : program.declarations) {
        if (auto* sd = dynamic_cast<const StructDefNode*>(decl.get())) {
            std::vector<Type*> fieldTypes;
            for (auto& f : sd->fields)
                fieldTypes.push_back(llvmType(f.type));
            StructType* st = StructType::create(ctx, fieldTypes, sd->name);
            structTypes[sd->name] = st;
            structFields[sd->name] = sd->fields;
            // Pré-declara métodos como NomeTipo__metodo(NomeTipo* self, ...)
            for (auto& m : sd->methods) {
                std::vector<Type*> paramTypes;
                paramTypes.push_back(PointerType::getUnqual(ctx)); // self
                for (auto& p : m.params)
                    paramTypes.push_back(llvmType(p.type));
                FunctionType* ft = FunctionType::get(llvmType(m.returnType), paramTypes, false);
                Function::Create(ft, Function::ExternalLinkage,
                                 sd->name + "__" + m.name, llvmModule.get());
            }
        }
        if (auto* fn = dynamic_cast<const FunctionNode*>(decl.get())) {
            std::vector<Type*> paramTypes;
            for (auto& p : fn->params)
                paramTypes.push_back(llvmType(p.type));
            FunctionType* ft = FunctionType::get(llvmType(fn->returnType), paramTypes, false);
            Function::Create(ft, Function::ExternalLinkage, fn->name, llvmModule.get());
        }
    }

    // Segunda passagem: gera tudo
    for (auto& decl : program.declarations) {
        if (auto* fn = dynamic_cast<const FunctionNode*>(decl.get()))
            codegenFunction(fn);
        else if (auto* vd = dynamic_cast<const VarDeclNode*>(decl.get()))
            codegenGlobalVar(vd);
        else if (auto* ad = dynamic_cast<const ArrayDeclNode*>(decl.get()))
            codegenGlobalArray(ad);
        else if (auto* sd = dynamic_cast<const StructVarDeclNode*>(decl.get())) {
            // Variável global de struct
            auto it = structTypes.find(sd->typeName);
            if (it == structTypes.end()) {
                std::cerr << "error: struct type '" << sd->typeName << "' not declared\n";
                exit(1);
            }
            StructType* st = it->second;
            Constant* init = ConstantAggregateZero::get(st);
            auto* gv = new GlobalVariable(*llvmModule, st, false,
                                          GlobalValue::ExternalLinkage, init, sd->varName);
            globalStructs[sd->varName] = {gv, sd->typeName};
        }
        // StructDefNode: gera os corpos dos métodos
        else if (auto* sd = dynamic_cast<const StructDefNode*>(decl.get())) {
            StructType* st = structTypes[sd->name];
            for (auto& m : sd->methods) {
                std::string funcName = sd->name + "__" + m.name;
                Function* func = llvmModule->getFunction(funcName);

                // Nomeia os parâmetros: self + params do método
                size_t pi = 0;
                for (auto& arg : func->args()) {
                    if (pi == 0) arg.setName("self");
                    else         arg.setName(m.params[pi - 1].name);
                    pi++;
                }

                BasicBlock* bb = BasicBlock::Create(ctx, "entry", func);
                builder.SetInsertPoint(bb);

                localValues.clear();
                localArrays.clear();
                localStructs.clear();

                // self é ponteiro para o struct — expõe campos como variáveis locais via GEP
                Value* selfPtr = func->arg_begin();
                auto& fields = structFields[sd->name];
                for (int fi = 0; fi < (int)fields.size(); fi++) {
                    Value* gep = builder.CreateStructGEP(st, selfPtr, fi, fields[fi].name);
                    // Guarda o GEP como AllocaInst* falso não funciona —
                    // usamos um mapa separado para self fields
                    // Hack limpo: cria alloca local e copia o valor de entrada
                    AllocaInst* alloca = createEntryAlloca(func, fields[fi].name, llvmType(fields[fi].type));
                    Value* loaded = builder.CreateLoad(llvmType(fields[fi].type), gep, fields[fi].name);
                    builder.CreateStore(loaded, alloca);
                    localValues[fields[fi].name] = alloca;
                }

                // Parâmetros normais
                pi = 1;
                for (auto& arg : func->args()) {
                    if (pi == 1) { pi++; continue; } // pula self
                    AllocaInst* alloca = createEntryAlloca(func, std::string(arg.getName()), arg.getType());
                    builder.CreateStore(&arg, alloca);
                    localValues[std::string(arg.getName())] = alloca;
                    pi++;
                }

                // Gera corpo
                for (auto& stmt : m.body)
                    codegenStmt(stmt.get(), func);

                // Escreve campos modificados de volta no struct (self)
                for (int fi = 0; fi < (int)fields.size(); fi++) {
                    auto it = localValues.find(fields[fi].name);
                    if (it != localValues.end()) {
                        Value* gep = builder.CreateStructGEP(st, selfPtr, fi, fields[fi].name + ".ptr");
                        Value* val = builder.CreateLoad(llvmType(fields[fi].type), it->second, fields[fi].name);
                        builder.CreateStore(val, gep);
                    }
                }

                if (!builder.GetInsertBlock()->getTerminator()) {
                    if (m.returnType == DataType::Void)
                        builder.CreateRetVoid();
                    else
                        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(ctx), 0));
                }
                verifyFunction(*func);
            }
        }
        // StructDefNode já foi processado na primeira passagem
    }

    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmPrinters();

    auto targetTriple = sys::getDefaultTargetTriple();
    // Garante x86_64 — substitui i386/i686 se o default vier como 32 bits
    if (targetTriple.find("i386") != std::string::npos ||
        targetTriple.find("i686") != std::string::npos) {
        targetTriple = "x86_64-" + targetTriple.substr(targetTriple.find('-') + 1);
    }
    llvmModule->setTargetTriple(targetTriple);

    std::string error;
    auto target = TargetRegistry::lookupTarget(targetTriple, error);
    if (!target) {
        std::cerr << "Target error: " << error << "\n";
        exit(1);
    }

    TargetOptions opt;
    auto* targetMachine = target->createTargetMachine(
        targetTriple, "x86-64", "+sse2", opt, Reloc::PIC_);

    llvmModule->setDataLayout(targetMachine->createDataLayout());

    // ── Otimizações LLVM (-O2 / -O3) ─────────────────────────────────
    if (optLevel >= 2) {
        PassBuilder pb(targetMachine);
        LoopAnalysisManager     lam;
        FunctionAnalysisManager fam;
        CGSCCAnalysisManager    cgam;
        ModuleAnalysisManager   mam;
        pb.registerModuleAnalyses(mam);
        pb.registerCGSCCAnalyses(cgam);
        pb.registerFunctionAnalyses(fam);
        pb.registerLoopAnalyses(lam);
        pb.crossRegisterProxies(lam, fam, cgam, mam);
        OptimizationLevel lvl = (optLevel >= 3)
            ? OptimizationLevel::O3
            : OptimizationLevel::O2;
        ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(lvl);
        mpm.run(*llvmModule, mam);
    }

    std::error_code ec;
    raw_fd_ostream dest(outputFile, ec, sys::fs::OF_None);
    if (ec) {
        std::cerr << "The file could not be opened: " << ec.message() << "\n";
        exit(1);
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr,
                                            CodeGenFileType::ObjectFile)) {
        std::cerr << "TargetMachine does not support object file output.\n";
        exit(1);
    }

    pass.run(*llvmModule);
    dest.flush();

}