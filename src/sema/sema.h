#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../ast/ast.h"
#include "type.h"

struct Diag {
    int code;
    int line;
    std::string message;
};

struct AnalysisResult {
    std::vector<Diag> diagnostics;
    std::unordered_map<const Expr *, Type> exprTypes;
    std::unordered_map<const Expr *, Type> typeOperands;
    std::unordered_map<const Stmt *, Type> declarationTypes;
    std::unordered_set<const Expr *> nativeObjectRefs;

    // Set/Add/Sub statements targeting explicitly declared C objects. The
    // target type is retained so codegen performs the conversion sema checked.
    std::unordered_map<const Stmt *, Type> nativeMutationTypes;
};

class Sema {
public:
    AnalysisResult analyze(const std::vector<Stmt *> &program);
    std::vector<Diag> check(const std::vector<Stmt *> &program);

private:
    struct Symbol {
        Type type;
        bool nativeObject = false;
    };

    std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    std::unordered_map<std::string, std::vector<Type>> procTable_;
    AnalysisResult *analysis_ = nullptr;

    void enterScope();
    void leaveScope();
    Symbol *findVar(const std::string &name);
    std::pair<Symbol, bool> lookupVar(const std::string &name, int line, std::vector<Diag> &diags);
    bool declareVar(const std::string &name, Type type, bool nativeObject,
                    int line, std::vector<Diag> &diags);
    Type resolveTypeSpec(const TypeSpec &spec) const;
    Type inferExpr(const Expr *e, int line, std::vector<Diag> &diags);
    void checkStmt(const Stmt *s, std::vector<Diag> &diags);
};
