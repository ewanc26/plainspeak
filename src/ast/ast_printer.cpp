#include "ast_printer.h"
#include <cstdio>
#include <sstream>

namespace {

std::string printTypeSpec(TypeSpec type) {
    switch (type.kind) {
        case TypeSpecKind::Void: return "void";
        case TypeSpecKind::Boolean: return "boolean";
        case TypeSpecKind::Character: return "character";
        case TypeSpecKind::SignedCharacter: return "signed character";
        case TypeSpecKind::UnsignedCharacter: return "unsigned character";
        case TypeSpecKind::ShortInteger: return "short integer";
        case TypeSpecKind::UnsignedShortInteger: return "unsigned short integer";
        case TypeSpecKind::Integer: return "integer";
        case TypeSpecKind::UnsignedInteger: return "unsigned integer";
        case TypeSpecKind::LongInteger: return "long integer";
        case TypeSpecKind::UnsignedLongInteger: return "unsigned long integer";
        case TypeSpecKind::LongLongInteger: return "long long integer";
        case TypeSpecKind::UnsignedLongLongInteger: return "unsigned long long integer";
        case TypeSpecKind::Float: return "float";
        case TypeSpecKind::Decimal: return "decimal";
        case TypeSpecKind::LongDecimal: return "long decimal";
    }
    return "<unknown type>";
}

std::string printExpr(const Expr *e) {
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) return std::to_string(node.value);
        else if constexpr (std::is_same_v<T, BoolLit>) return node.value ? "true" : "false";
        else if constexpr (std::is_same_v<T, FloatLit>) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.17g", node.value);
            return buf;
        } else if constexpr (std::is_same_v<T, StringLit>) return "\"" + node.value + "\"";
        else if constexpr (std::is_same_v<T, VarRef>) return node.name;
        else if constexpr (std::is_same_v<T, ListExpr>) {
            std::string out = "List with ";
            for (size_t i = 0; i < node.items.size(); ++i) {
                if (i) out += " followed by ";
                out += printExpr(node.items[i]);
            }
            return out + " done";
        } else if constexpr (std::is_same_v<T, EmptyListExpr>) {
            const char *kind = node.elementKind == ListElementKind::Number ? "numbers"
                             : node.elementKind == ListElementKind::Decimal ? "decimals"
                                                                           : "strings";
            return std::string("Empty list of ") + kind;
        } else if constexpr (std::is_same_v<T, ItemExpr>) {
            return "Item at " + printExpr(node.index) + " in " + printExpr(node.list);
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            return "Length of " + printExpr(node.operand);
        } else if constexpr (std::is_same_v<T, SizeOfTypeExpr>) {
            return "Size of type " + printTypeSpec(node.type);
        } else if constexpr (std::is_same_v<T, AlignOfTypeExpr>) {
            return "Alignment of type " + printTypeSpec(node.type);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            std::string out = "Call " + node.name;
            if (!node.args.empty()) {
                out += " with ";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i) out += ", ";
                    out += printExpr(node.args[i]);
                }
            }
            return out + " done";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            const char *op = node.op == BinOp::Add ? "plus"
                          : node.op == BinOp::Sub ? "minus"
                          : node.op == BinOp::Mul ? "times"
                          : node.op == BinOp::Div ? "divided by"
                          : node.op == BinOp::Mod ? "mod"
                          : node.op == BinOp::Gt  ? "is greater than"
                          : node.op == BinOp::Lt  ? "is less than"
                          : node.op == BinOp::Eq  ? "is equal to"
                          : node.op == BinOp::Ne  ? "is not equal to"
                          : node.op == BinOp::Ge  ? "is greater than or equal to"
                          : node.op == BinOp::Le  ? "is less than or equal to"
                          : node.op == BinOp::And ? "and" : "or";
            return "(" + printExpr(node.lhs) + " " + std::string(op) + " " + printExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return std::string("(") + (node.op == UnaryOp::Not ? "not" : "minus") + " " + printExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, MathCallExpr>) {
            return node.func + " of " + printExpr(node.arg);
        } else if constexpr (std::is_same_v<T, PowExpr>) {
            return "(" + printExpr(node.base) + " to the power of " + printExpr(node.exp) + ")";
        }
        return "<unknown expr>";
    }, e->node);
}

std::string printStmt(const Stmt *s) {
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) return "Say " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, SetStmt>) return "Set " + node.name + " to " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, AddStmt>) return "Add " + printExpr(node.expr) + " to " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, SubStmt>) return "Subtract " + printExpr(node.expr) + " from " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReadStmt>) return "Read " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReadFloatStmt>) return "ReadFloat " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, AppendStmt>) return "Append " + printExpr(node.expr) + " to " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReplaceItemStmt>) return "Replace item at " + printExpr(node.index) + " in " + node.varName + " with " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, RemoveItemStmt>) return "Remove item at " + printExpr(node.index) + " from " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, CommentStmt>) return "(" + node.text + ") ";
        else if constexpr (std::is_same_v<T, RepeatStmt>) {
            std::string out = "Repeat " + printExpr(node.count) + ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End repeat. ";
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            std::string out = "If " + printExpr(node.cond) + " then: ";
            for (Stmt *inner : node.thenBody) out += printStmt(inner);
            if (!node.elseBody.empty()) {
                out += "Else: ";
                for (Stmt *inner : node.elseBody) out += printStmt(inner);
            }
            return out + "End if. ";
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            std::string out = "While " + printExpr(node.cond) + ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End while. ";
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            std::string out = "For each " + node.itemName + " in " + printExpr(node.list) + ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End for. ";
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            std::string out = "Call " + node.name;
            if (!node.args.empty()) {
                out += " with ";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i) out += ", ";
                    out += printExpr(node.args[i]);
                }
            }
            return out + " done. ";
        } else if constexpr (std::is_same_v<T, ReturnStmt>) return "Return " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::string out = "Procedure " + node.name;
            if (!node.params.empty()) {
                out += " takes ";
                for (size_t i = 0; i < node.params.size(); ++i) {
                    if (i) out += ", ";
                    out += node.params[i];
                }
            }
            out += ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End procedure. ";
        }
        return "<unknown stmt> ";
    }, s->node);
}

} // namespace

std::string printAST(const std::vector<Stmt *> &program) {
    std::string out;
    for (Stmt *s : program) out += printStmt(s);
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out + "\n";
}
