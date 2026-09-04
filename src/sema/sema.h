#pragma once
#include <optional>
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

struct ProcedureSignature {
    std::vector<Type> parameterTypes;
    Type returnType = Type::number();
    bool nativeTyped = false;
};

struct AggregateFieldInfo {
    std::string name;
    Type type;
    std::optional<std::size_t> bitWidth;
    bool flexibleArray = false;

    bool isUnnamedBitField() const { return bitWidth.has_value() && name.empty(); }
};

struct StructureInfo {
    std::vector<AggregateFieldInfo> fields;
    bool complete = false;
    bool hasFlexibleArray = false;
};

struct EnumerationInfo {
    std::vector<std::pair<std::string, long>> enumerators;
    bool complete = false;
};

struct AnalysisResult {
    std::vector<Diag> diagnostics;
    std::unordered_map<const Expr *, Type> exprTypes;
    std::unordered_map<const Expr *, Type> typeOperands;
    std::unordered_map<const Stmt *, Type> declarationTypes;
    std::unordered_map<std::string, ProcedureSignature> procedureSignatures;
    std::unordered_map<std::string, StructureInfo> structures;
    std::unordered_map<std::string, StructureInfo> unions;
    std::unordered_map<std::string, EnumerationInfo> enumerations;
    std::unordered_map<const Stmt *, std::vector<AggregateFieldInfo>> structureFields;
    std::unordered_map<const Stmt *, std::vector<AggregateFieldInfo>> unionFields;
    std::unordered_map<const Stmt *, std::vector<std::pair<std::string, long>>> enumerationValues;
    std::unordered_set<const Expr *> nativeObjectRefs;
    std::unordered_set<const Expr *> bitFieldExprs;

    // Existing Set/Add/Sub statements whose target is an explicitly declared
    // C object. C itself performs the already-checked assignment conversion.
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
    std::unordered_map<std::string, ProcedureSignature> procTable_;
    std::unordered_map<std::string, StructureInfo> structureTable_;
    std::unordered_map<std::string, StructureInfo> unionTable_;
    std::unordered_map<std::string, EnumerationInfo> enumerationTable_;
    std::optional<ProcedureSignature> currentProcedure_;
    AnalysisResult *analysis_ = nullptr;

    void enterScope();
    void leaveScope();
    Symbol *findVar(const std::string &name);
    std::pair<Symbol, bool> lookupVar(const std::string &name, int line, std::vector<Diag> &diags);
    bool declareVar(const std::string &name, Type type, bool nativeObject,
                    int line, std::vector<Diag> &diags);
    Type resolveTypeSpec(const TypeSpec &spec) const;
    bool validateTypeQualifiers(const Type &type, int line, std::vector<Diag> &diags) const;
    bool isCompleteObjectType(const Type &type) const;
    bool hasConstSubobject(const Type &type) const;
    bool isModifiableObjectType(const Type &type) const;
    bool containsFlexibleArray(const Type &type) const;
    const AggregateFieldInfo *findAggregateField(const Type &base, const std::string &name) const;
    Type inferExpr(const Expr *e, int line, std::vector<Diag> &diags);
    void checkStmt(const Stmt *s, std::vector<Diag> &diags);
};
