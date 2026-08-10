#include "sema.h"
#include <unordered_map>
#include <unordered_set>

std::string typeToString(Type t) {
    if (t == Type::Int) return "number";
    if (t == Type::Double) return "decimal";
    return "string";
}

std::vector<Diag> Sema::check(const std::vector<Stmt *> &program) {
    std::vector<Diag> diags;
    scopes_.clear();
    scopes_.emplace_back();

    for (Stmt *s : program) checkStmt(s, diags);

    scopes_.pop_back();
    return diags;
}

void Sema::enterScope() { scopes_.emplace_back(); }
void Sema::leaveScope() { scopes_.pop_back(); }

std::pair<Type, bool> Sema::lookupVar(const std::string &name, int line, std::vector<Diag> &diags) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if (it->count(name)) return {(*it)[name], true};
    }
    diags.push_back({1, line, "I don't know what to do with \"" + name + "\" — it is used here but never declared. Use Set to create it first."});
    return {Type::Int, false};
}

bool Sema::declareVar(const std::string &name, Type type, int line, std::vector<Diag> &diags) {
    auto &current = scopes_.back();
    if (current.count(name)) {
        diags.push_back({6, line, "variable \"" + name + "\" is already declared in this scope"});
        return false;
    }
    current[name] = type;
    return true;
}

Type Sema::inferExpr(const Expr *e, int line, std::vector<Diag> &diags) {
    return std::visit([&](auto &&node) -> Type {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) return Type::Int;
        else if constexpr (std::is_same_v<T, BoolLit>) return Type::Int;
        else if constexpr (std::is_same_v<T, FloatLit>) return Type::Double;
        else if constexpr (std::is_same_v<T, StringLit>) return Type::String;
        else if constexpr (std::is_same_v<T, VarRef>) {
            auto [type, found] = lookupVar(node.name, line, diags);
            return type;
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Type lhs = inferExpr(node.lhs, line, diags);
            Type rhs = inferExpr(node.rhs, line, diags);
            if (node.op == BinOp::Add) {
                bool lhsStr = lhs == Type::String;
                bool rhsStr = rhs == Type::String;
                bool lhsNum = lhs == Type::Int || lhs == Type::Double;
                bool rhsNum = rhs == Type::Int || rhs == Type::Double;
                if (!((lhsStr && rhsStr) || (lhsNum && rhsNum) || (lhsStr && rhsNum) || (lhsNum && rhsStr))) {
                    diags.push_back({2, line, "I can't add a " + typeToString(lhs) + " to a " + typeToString(rhs) + "."});
                }
                if (lhsStr || rhsStr) return Type::String;
                if (lhs == Type::Double || rhs == Type::Double) return Type::Double;
                return Type::Int;
            } else if (node.op == BinOp::Sub || node.op == BinOp::Mul || node.op == BinOp::Div || node.op == BinOp::Mod) {
                if (lhs != Type::Int && lhs != Type::Double) {
                    diags.push_back({2, line, "I can't do arithmetic on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                if (rhs != Type::Int && rhs != Type::Double) {
                    diags.push_back({2, line, "I can't do arithmetic on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                if (lhs == Type::Double || rhs == Type::Double) return Type::Double;
                return Type::Int;
            } else if (node.op == BinOp::And || node.op == BinOp::Or) {
                if (lhs != Type::Int || rhs != Type::Int) {
                    diags.push_back({2, line, "I can't do logical " + std::string(node.op == BinOp::And ? "and" : "or") + " on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                return Type::Int;
            } else {
                if (lhs != rhs && !((lhs == Type::Int && rhs == Type::Double) || (lhs == Type::Double && rhs == Type::Int))) {
                    diags.push_back({4, line, "I can't compare a " + typeToString(lhs) + " with a " + typeToString(rhs) + ". Both sides must be the same type."});
                }
                return Type::Int;
            }
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Type rhs = inferExpr(node.rhs, line, diags);
            if (node.op == UnaryOp::Neg) {
                if (rhs != Type::Int && rhs != Type::Double) {
                    diags.push_back({2, line, "I can't negate a " + typeToString(rhs) + "."});
                }
                return rhs;
            }
            if (rhs != Type::Int) {
                diags.push_back({2, line, "I can't apply not to a " + typeToString(rhs) + ". It must be a number."});
            }
            return Type::Int;
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (!procTable_.count(node.name)) {
                diags.push_back({7, line, "I don't know what to do with \"" + node.name + "\" — it is used here but never defined. Use Procedure to create it first."});
            } else {
                const auto &expected = procTable_[node.name];
                if (node.args.size() != expected.size()) {
                    diags.push_back({8, line, "Call to \"" + node.name + "\" expects " + std::to_string(expected.size()) + " arguments but got " + std::to_string(node.args.size()) + "."});
                }
                for (size_t i = 0; i < node.args.size(); ++i) {
                    inferExpr(node.args[i], line, diags);
                }
            }
            return Type::Int;
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            Type operand = inferExpr(node.operand, line, diags);
            if (operand != Type::String) {
                diags.push_back({2, line, "I can't get the length of a " + typeToString(operand) + ". It must be a string."});
            }
            return Type::Int;
        } else if constexpr (std::is_same_v<T, MathCallExpr>) {
            Type arg = inferExpr(node.arg, line, diags);
            if (arg != Type::Int && arg != Type::Double) {
                diags.push_back({2, line, "I can't apply " + node.func + " to a " + typeToString(arg) + "."});
            }
            return Type::Double;
        } else if constexpr (std::is_same_v<T, PowExpr>) {
            Type base = inferExpr(node.base, line, diags);
            Type exp = inferExpr(node.exp, line, diags);
            if ((base != Type::Int && base != Type::Double) || (exp != Type::Int && exp != Type::Double)) {
                diags.push_back({2, line, "I can't raise a " + typeToString(base) + " to the power of a " + typeToString(exp) + "."});
            }
            return Type::Double;
        }
        return Type::Int;
    }, e->node);
}

void Sema::checkStmt(const Stmt *s, std::vector<Diag> &diags) {
    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            inferExpr(node.expr, s->line, diags);
        } else if constexpr (std::is_same_v<T, SetStmt>) {
            Type exprType = inferExpr(node.expr, s->line, diags);
            declareVar(node.name, exprType, s->line, diags);
        } else if constexpr (std::is_same_v<T, AddStmt>) {
            auto [varType, found] = lookupVar(node.varName, s->line, diags);
            if (found) {
                Type exprType = inferExpr(node.expr, s->line, diags);
                if (varType != exprType) {
                    diags.push_back({3, s->line, "I can't add a " + typeToString(exprType) + " to \"" + node.varName + "\" which is a " + typeToString(varType) + "."});
                }
            }
        } else if constexpr (std::is_same_v<T, SubStmt>) {
            auto [varType, found] = lookupVar(node.varName, s->line, diags);
            if (found) {
                Type exprType = inferExpr(node.expr, s->line, diags);
                if (varType != exprType) {
                    diags.push_back({3, s->line, "I can't subtract a " + typeToString(exprType) + " from \"" + node.varName + "\" which is a " + typeToString(varType) + "."});
                }
            }
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            declareVar(node.varName, Type::Int, s->line, diags);
        } else if constexpr (std::is_same_v<T, ReadFloatStmt>) {
            declareVar(node.varName, Type::Double, s->line, diags);
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            Type countType = inferExpr(node.count, s->line, diags);
            if (countType != Type::Int) {
                diags.push_back({5, s->line, "Repeat needs a number of times, not a " + typeToString(countType) + "."});
            }
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            inferExpr(node.cond, s->line, diags);
            enterScope();
            for (Stmt *inner : node.thenBody) checkStmt(inner, diags);
            leaveScope();
            if (!node.elseBody.empty()) {
                enterScope();
                for (Stmt *inner : node.elseBody) checkStmt(inner, diags);
                leaveScope();
            }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            Type condType = inferExpr(node.cond, s->line, diags);
            if (condType != Type::Int && condType != Type::Double) {
                diags.push_back({5, s->line, "While needs a number condition, not a " + typeToString(condType) + "."});
            }
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        } else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::vector<Type> paramTypes(node.params.size(), Type::Int);
            procTable_[node.name] = paramTypes;
            enterScope();
            for (size_t i = 0; i < node.params.size(); ++i) {
                declareVar(node.params[i], paramTypes[i], s->line, diags);
            }
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            if (!procTable_.count(node.name)) {
                diags.push_back({7, s->line, "I don't know what to do with \"" + node.name + "\" — it is used here but never defined. Use Procedure to create it first."});
            } else {
                const auto &expected = procTable_[node.name];
                if (node.args.size() != expected.size()) {
                    diags.push_back({8, s->line, "Call to \"" + node.name + "\" expects " + std::to_string(expected.size()) + " arguments but got " + std::to_string(node.args.size()) + "."});
                }
                for (size_t i = 0; i < node.args.size(); ++i) {
                    inferExpr(node.args[i], s->line, diags);
                }
            }
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            inferExpr(node.expr, s->line, diags);
        } else if constexpr (std::is_same_v<T, CommentStmt>) {
        }
    }, s->node);
}
