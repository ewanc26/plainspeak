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

    // Resolved semantic type for every expression that sema visited.
    std::unordered_map<const Expr *, Type> exprTypes;

    // Resolved operand type for type/object queries such as sizeof/_Alignof.
    std::unordered_map<const Expr *, Type> typeOperands;

    // Resolved native C type for each explicit Declare statement.
    std::unordered_map<const Stmt *, Type> declarationTypes;

    // VarRef nodes that bind to an explicit native C object rather than a
    // legacy boxed PsValue variable.
    std::unordered_set<const Expr *> nativeObjectRefs;

    // Existing Set/Add/Sub statements whose target is a native C object.
    std::unordered_set<const Stmt *> nativeMutationTargets;
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
