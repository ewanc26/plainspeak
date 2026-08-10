#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "../ast/ast.h"

enum class Type { Int, String };

struct Diag {
    int code;
    int line;
    std::string message;
};

class Sema {
public:
    std::vector<Diag> check(const std::vector<Stmt *> &program);

private:
    std::vector<std::unordered_map<std::string, Type>> scopes_;

    void enterScope();
    void leaveScope();
    std::pair<Type, bool> lookupVar(const std::string &name, int line, std::vector<Diag> &diags);
    bool declareVar(const std::string &name, Type type, int line, std::vector<Diag> &diags);
    Type inferExpr(const Expr *e, int line, std::vector<Diag> &diags);
    void checkStmt(const Stmt *s, std::vector<Diag> &diags);
};
