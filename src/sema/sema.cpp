#include "sema.h"
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace {

bool isNumeric(const Type &t) { return t.isNumeric(); }
bool isList(const Type &t) { return t.isList(); }

Type listElementType(const Type &t) {
    if (t.kind == TypeKind::List && t.elementType) return *t.elementType;
    return Type::number();
}

Type listTypeFor(Type t) { return Type::listOf(std::move(t)); }

Type listTypeFor(ListElementKind kind) {
    if (kind == ListElementKind::Decimal) return Type::listOf(Type::decimal());
    if (kind == ListElementKind::String) return Type::listOf(Type::string());
    return Type::listOf(Type::number());
}

std::string integerName(const Type &t) {
    if (t.integerRank == IntegerRank::Char) {
        if (t.charSignedness == CharSignedness::Plain) return "character";
        if (t.charSignedness == CharSignedness::Unsigned) return "unsigned character";
        return "signed character";
    }

    std::string name;
    if (t.isUnsigned) name += "unsigned ";
    switch (t.integerRank) {
        case IntegerRank::Char: break;
        case IntegerRank::Short: name += "short integer"; break;
        case IntegerRank::Int: name += "integer"; break;
        case IntegerRank::Long: name += "number"; break;
        case IntegerRank::LongLong: name += "long long integer"; break;
    }
    return name;
}

std::string typeToString(const Type &t) {
    switch (t.kind) {
        case TypeKind::Void: return "void";
        case TypeKind::Boolean: return "boolean";
        case TypeKind::Integer: return integerName(t);
        case TypeKind::Floating:
            if (t.floatingRank == FloatingRank::Float) return "float";
            if (t.floatingRank == FloatingRank::LongDouble) return "long decimal";
            return "decimal";
        case TypeKind::String: return "string";
        case TypeKind::List: return "list of " + typeToString(listElementType(t)) + "s";
        case TypeKind::Pointer:
            return "pointer to " + (t.elementType ? typeToString(*t.elementType) : std::string("unknown"));
        case TypeKind::Array:
            return "array of " + (t.elementType ? typeToString(*t.elementType) : std::string("unknown"));
        case TypeKind::Function: return "function";
        case TypeKind::Structure: return "structure " + t.tag;
        case TypeKind::Union: return "union " + t.tag;
        case TypeKind::Enumeration: return "enumeration " + t.tag;
        case TypeKind::BitInt:
            return std::string(t.isUnsigned ? "unsigned " : "") + "bit integer of width " + std::to_string(t.bitWidth);
        case TypeKind::Nullptr: return "null pointer";
    }
    return "unknown type";
}

} // namespace

std::vector<Diag> Sema::check(const std::vector<Stmt *> &program) {
    std::vector<Diag> diags;
    scopes_.clear();
    procTable_.clear();
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
    return {Type::number(), false};
}

bool Sema::declareVar(const std::string &name, Type type, int line, std::vector<Diag> &diags) {
    auto &current = scopes_.back();
    if (current.count(name)) {
        diags.push_back({6, line, "variable \"" + name + "\" is already declared in this scope"});
        return false;
    }
    current[name] = std::move(type);
    return true;
}

Type Sema::inferExpr(const Expr *e, int line, std::vector<Diag> &diags) {
    return std::visit([&](auto &&node) -> Type {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) return Type::number();
        else if constexpr (std::is_same_v<T, BoolLit>) return Type::number();
        else if constexpr (std::is_same_v<T, FloatLit>) return Type::decimal();
        else if constexpr (std::is_same_v<T, StringLit>) return Type::string();
        else if constexpr (std::is_same_v<T, VarRef>) {
            auto [type, found] = lookupVar(node.name, line, diags);
            (void)found;
            return type;
        }
        else if constexpr (std::is_same_v<T, ListExpr>) {
            Type first = inferExpr(node.items.front(), line, diags);
            if (isList(first)) {
                diags.push_back({9, line, "Lists can't contain other lists. Use numbers, decimals, or strings as list items."});
                first = Type::number();
            }
            for (size_t i = 1; i < node.items.size(); ++i) {
                Type item = inferExpr(node.items[i], line, diags);
                if (isList(item) || item != first) {
                    diags.push_back({9, line, "Every item in a list must have the same type; this list starts with a " +
                                             typeToString(first) + " but also contains a " + typeToString(item) + "."});
                }
            }
            return listTypeFor(first);
        }
        else if constexpr (std::is_same_v<T, EmptyListExpr>) {
            return listTypeFor(node.elementKind);
        }
        else if constexpr (std::is_same_v<T, ItemExpr>) {
            Type index = inferExpr(node.index, line, diags);
            if (index != Type::number()) {
                diags.push_back({11, line, "A list position must be a whole number, not a " + typeToString(index) + "."});
            }
            Type list = inferExpr(node.list, line, diags);
            if (!isList(list)) {
                diags.push_back({10, line, "I can only take an item from a list, not a " + typeToString(list) + "."});
                return Type::number();
            }
            return listElementType(list);
        }
        else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Type lhs = inferExpr(node.lhs, line, diags);
            Type rhs = inferExpr(node.rhs, line, diags);
            if (node.op == BinOp::Add) {
                bool lhsStr = lhs.kind == TypeKind::String;
                bool rhsStr = rhs.kind == TypeKind::String;
                bool lhsNum = isNumeric(lhs);
                bool rhsNum = isNumeric(rhs);
                if (!((lhsStr && rhsStr) || (lhsNum && rhsNum) || (lhsStr && rhsNum) || (lhsNum && rhsStr))) {
                    diags.push_back({2, line, "I can't add a " + typeToString(lhs) + " to a " + typeToString(rhs) + "."});
                }
                if (lhsStr || rhsStr) return Type::string();
                if (lhs.isFloating() || rhs.isFloating()) return Type::decimal();
                return Type::number();
            } else if (node.op == BinOp::Sub || node.op == BinOp::Mul || node.op == BinOp::Div || node.op == BinOp::Mod) {
                if (!isNumeric(lhs) || !isNumeric(rhs)) {
                    diags.push_back({2, line, "I can't do arithmetic on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                if (lhs.isFloating() || rhs.isFloating()) return Type::decimal();
                return Type::number();
            } else if (node.op == BinOp::And || node.op == BinOp::Or) {
                if (lhs != Type::number() || rhs != Type::number()) {
                    diags.push_back({2, line, "I can't do logical " + std::string(node.op == BinOp::And ? "and" : "or") + " on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                return Type::number();
            } else {
                if (isList(lhs) || isList(rhs) ||
                    (lhs != rhs && !(isNumeric(lhs) && isNumeric(rhs)))) {
                    diags.push_back({4, line, "I can't compare a " + typeToString(lhs) + " with a " + typeToString(rhs) + ". Both sides must be comparable scalar values."});
                }
                return Type::number();
            }
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Type rhs = inferExpr(node.rhs, line, diags);
            if (node.op == UnaryOp::Neg) {
                if (!isNumeric(rhs)) diags.push_back({2, line, "I can't negate a " + typeToString(rhs) + "."});
                return rhs;
            }
            if (rhs != Type::number()) {
                diags.push_back({2, line, "I can't apply not to a " + typeToString(rhs) + ". It must be a number."});
            }
            return Type::number();
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            if (!procTable_.count(node.name)) {
                diags.push_back({7, line, "I don't know what to do with \"" + node.name + "\" — it is used here but never defined. Use Procedure to create it first."});
            } else {
                const auto &expected = procTable_[node.name];
                if (node.args.size() != expected.size()) {
                    diags.push_back({8, line, "Call to \"" + node.name + "\" expects " + std::to_string(expected.size()) + " arguments but got " + std::to_string(node.args.size()) + "."});
                }
                for (Expr *arg : node.args) inferExpr(arg, line, diags);
            }
            return Type::number();
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            Type operand = inferExpr(node.operand, line, diags);
            if (operand != Type::string() && !isList(operand)) {
                diags.push_back({2, line, "I can't get the length of a " + typeToString(operand) + ". It must be a string or a list."});
            }
            return Type::number();
        } else if constexpr (std::is_same_v<T, SizeOfTypeExpr>) {
            if (node.type.kind == TypeSpecKind::Void) {
                diags.push_back({12, line, "I can't ask for the size of void because void is not an object type."});
            }
            return Type::number();
        } else if constexpr (std::is_same_v<T, AlignOfTypeExpr>) {
            if (node.type.kind == TypeSpecKind::Void) {
                diags.push_back({12, line, "I can't ask for the alignment of void because void is not an object type."});
            }
            return Type::number();
        } else if constexpr (std::is_same_v<T, MathCallExpr>) {
            Type arg = inferExpr(node.arg, line, diags);
            if (!isNumeric(arg)) diags.push_back({2, line, "I can't apply " + node.func + " to a " + typeToString(arg) + "."});
            return Type::decimal();
        } else if constexpr (std::is_same_v<T, PowExpr>) {
            Type base = inferExpr(node.base, line, diags);
            Type exp = inferExpr(node.exp, line, diags);
            if (!isNumeric(base) || !isNumeric(exp)) {
                diags.push_back({2, line, "I can't raise a " + typeToString(base) + " to the power of a " + typeToString(exp) + "."});
            }
            return Type::decimal();
        }
        return Type::number();
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
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (found && varType != exprType) {
                diags.push_back({3, s->line, "I can't add a " + typeToString(exprType) + " to \"" + node.varName + "\" which is a " + typeToString(varType) + "."});
            }
        } else if constexpr (std::is_same_v<T, SubStmt>) {
            auto [varType, found] = lookupVar(node.varName, s->line, diags);
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (found && varType != exprType) {
                diags.push_back({3, s->line, "I can't subtract a " + typeToString(exprType) + " from \"" + node.varName + "\" which is a " + typeToString(varType) + "."});
            }
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            declareVar(node.varName, Type::number(), s->line, diags);
        } else if constexpr (std::is_same_v<T, ReadFloatStmt>) {
            declareVar(node.varName, Type::decimal(), s->line, diags);
        } else if constexpr (std::is_same_v<T, AppendStmt>) {
            auto [listType, found] = lookupVar(node.varName, s->line, diags);
            Type itemType = inferExpr(node.expr, s->line, diags);
            if (found && !isList(listType)) {
                diags.push_back({10, s->line, "I can only append to a list; \"" + node.varName + "\" is a " + typeToString(listType) + "."});
            } else if (found && itemType != listElementType(listType)) {
                diags.push_back({9, s->line, "I can't append a " + typeToString(itemType) + " to \"" + node.varName + "\" which is a " + typeToString(listType) + "."});
            }
        } else if constexpr (std::is_same_v<T, ReplaceItemStmt>) {
            Type indexType = inferExpr(node.index, s->line, diags);
            if (indexType != Type::number()) {
                diags.push_back({11, s->line, "A list position must be a whole number, not a " + typeToString(indexType) + "."});
            }
            auto [listType, found] = lookupVar(node.varName, s->line, diags);
            Type itemType = inferExpr(node.expr, s->line, diags);
            if (found && !isList(listType)) {
                diags.push_back({10, s->line, "I can only replace an item in a list; \"" + node.varName + "\" is a " + typeToString(listType) + "."});
            } else if (found && itemType != listElementType(listType)) {
                diags.push_back({9, s->line, "I can't put a " + typeToString(itemType) + " into \"" + node.varName + "\" which is a " + typeToString(listType) + "."});
            }
        } else if constexpr (std::is_same_v<T, RemoveItemStmt>) {
            Type indexType = inferExpr(node.index, s->line, diags);
            if (indexType != Type::number()) {
                diags.push_back({11, s->line, "A list position must be a whole number, not a " + typeToString(indexType) + "."});
            }
            auto [listType, found] = lookupVar(node.varName, s->line, diags);
            if (found && !isList(listType)) {
                diags.push_back({10, s->line, "I can only remove an item from a list; \"" + node.varName + "\" is a " + typeToString(listType) + "."});
            }
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            Type countType = inferExpr(node.count, s->line, diags);
            if (countType != Type::number()) {
                diags.push_back({5, s->line, "Repeat needs a whole number of times, not a " + typeToString(countType) + "."});
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
            if (!isNumeric(condType)) {
                diags.push_back({5, s->line, "While needs a number condition, not a " + typeToString(condType) + "."});
            }
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            Type listType = inferExpr(node.list, s->line, diags);
            Type itemType = Type::number();
            if (!isList(listType)) {
                diags.push_back({10, s->line, "For each needs a list to walk through, not a " + typeToString(listType) + "."});
            } else {
                itemType = listElementType(listType);
            }
            enterScope();
            declareVar(node.itemName, itemType, s->line, diags);
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        } else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::vector<Type> paramTypes(node.params.size(), Type::number());
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
                for (Expr *arg : node.args) inferExpr(arg, s->line, diags);
            }
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            inferExpr(node.expr, s->line, diags);
        } else if constexpr (std::is_same_v<T, CommentStmt>) {
        }
    }, s->node);
}
