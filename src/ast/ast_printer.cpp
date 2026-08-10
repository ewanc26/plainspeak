#include "ast_printer.h"
#include <sstream>

namespace {

std::string printExpr(const Expr *e) {
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) {
            return std::to_string(node.value);
        } else if constexpr (std::is_same_v<T, BoolLit>) {
            return node.value ? "true" : "false";
        } else if constexpr (std::is_same_v<T, StringLit>) {
            return "\"" + node.value + "\"";
        } else if constexpr (std::is_same_v<T, VarRef>) {
            return node.name;
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            return "Length of " + printExpr(node.operand);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            std::string args;
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) args += ", ";
                args += printExpr(node.args[i]);
            }
            return "Call " + node.name + " with " + args + " done";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            const char *op = node.op == BinOp::Add ? "plus"
                          : node.op == BinOp::Sub ? "minus"
                          : node.op == BinOp::Mul ? "times"
                          : node.op == BinOp::Div ? "divided by"
                          : node.op == BinOp::Gt  ? "is greater than"
                          : node.op == BinOp::Lt  ? "is less than"
                          : node.op == BinOp::Eq  ? "is equal to"
                          : node.op == BinOp::Ne  ? "is not equal to"
                          : node.op == BinOp::Ge  ? "is greater than or equal to"
                          : node.op == BinOp::Le  ? "is less than or equal to"
                          : node.op == BinOp::And ? "and"
                                                   : "or";
            return "(" + printExpr(node.lhs) + " " + std::string(op) + " " + printExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return std::string("(not ") + printExpr(node.rhs) + ")";
        }
        return "<unknown expr>";
    }, e->node);
}

std::string printStmt(const Stmt *s, int indent) {
    std::string pad(indent * 2, ' ');
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            return pad + "Say " + printExpr(node.expr) + ".\n";
        } else if constexpr (std::is_same_v<T, SetStmt>) {
            return pad + "Set " + node.name + " to " + printExpr(node.expr) + ".\n";
        } else if constexpr (std::is_same_v<T, AddStmt>) {
            return pad + "Add " + printExpr(node.expr) + " to " + node.varName + ".\n";
        } else if constexpr (std::is_same_v<T, SubStmt>) {
            return pad + "Subtract " + printExpr(node.expr) + " from " + node.varName + ".\n";
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            return pad + "Read " + node.varName + ".\n";
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            std::string out = pad + "Repeat " + printExpr(node.count) + " times:\n";
            for (Stmt *inner : node.body) out += printStmt(inner, indent + 1);
            out += pad + "End repeat.\n";
            return out;
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            std::string out = pad + "If " + printExpr(node.cond) + " then:\n";
            for (Stmt *inner : node.thenBody) out += printStmt(inner, indent + 1);
            if (!node.elseBody.empty()) {
                out += pad + "Else:\n";
                for (Stmt *inner : node.elseBody) out += printStmt(inner, indent + 1);
            }
            out += pad + "End if.\n";
            return out;
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            std::string out = pad + "While " + printExpr(node.cond) + ":\n";
            for (Stmt *inner : node.body) out += printStmt(inner, indent + 1);
            out += pad + "End while.\n";
            return out;
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            std::string args;
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) args += ", ";
                args += printExpr(node.args[i]);
            }
            return pad + "Call " + node.name + " with " + args + ".\n";
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            return pad + "Return " + printExpr(node.expr) + ".\n";
        } else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::string params;
            for (size_t i = 0; i < node.params.size(); ++i) {
                if (i > 0) params += ", ";
                params += node.params[i];
            }
            std::string out = pad + "Procedure " + node.name + " takes " + params + ":\n";
            for (Stmt *inner : node.body) out += printStmt(inner, indent + 1);
            out += pad + "End procedure.\n";
            return out;
        }
        return "<unknown stmt>\n";
    }, s->node);
}

}

std::string printAST(const std::vector<Stmt *> &program) {
    std::string out;
    for (Stmt *s : program) out += printStmt(s, 0);
    return out;
}
