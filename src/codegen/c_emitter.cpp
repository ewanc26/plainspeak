#include "c_emitter.h"
#include <set>
#include <sstream>
#include <variant>
#include "mangling.h"

namespace {

void collectVars(const std::vector<Stmt *> &stmts, std::set<std::string> &out) {
    for (Stmt *s : stmts) {
        std::visit([&](auto &&node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, SetStmt>) out.insert(node.name);
            else if constexpr (std::is_same_v<T, AddStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, RepeatStmt>) collectVars(node.body, out);
            else if constexpr (std::is_same_v<T, IfStmt>) { collectVars(node.thenBody, out); collectVars(node.elseBody, out); }
            else if constexpr (std::is_same_v<T, WhileStmt>) collectVars(node.body, out);
        }, s->node);
    }
}

std::string emitExpr(const Expr *e) {
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) {
            return "ps_int(" + std::to_string(node.value) + "L)";
        } else if constexpr (std::is_same_v<T, BoolLit>) {
            return "ps_int(" + std::string(node.value ? "1" : "0") + "L)";
        } else if constexpr (std::is_same_v<T, StringLit>) {
            std::string escaped;
            for (char c : node.value) {
                if (c == '"' || c == '\\') escaped += '\\';
                if (c == '\n') { escaped += "\\n"; continue; }
                escaped += c;
            }
            return "ps_str(\"" + escaped + "\")";
        } else if constexpr (std::is_same_v<T, VarRef>) {
            return mangle(node.name);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            const char *fn = node.op == BinOp::Add ? "ps_add"
                            : node.op == BinOp::Sub ? "ps_sub"
                            : node.op == BinOp::Mul ? "ps_mul"
                            : node.op == BinOp::Div ? "ps_div"
                            : node.op == BinOp::Gt  ? "ps_gt"
                            : node.op == BinOp::Lt  ? "ps_lt"
                            : node.op == BinOp::Eq  ? "ps_eq"
                            : node.op == BinOp::And ? "ps_and"
                                                     : "ps_or";
            return std::string(fn) + "(" + emitExpr(node.lhs) + ", " + emitExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            return std::string("ps_not(") + emitExpr(node.rhs) + ")";
        }
    }, e->node);
}

void emitStmt(const Stmt *s, std::ostream &out, std::string indent, int &loopCounter,
              const std::unordered_map<int, std::string> *sourceLines) {
    if (sourceLines) {
        auto it = sourceLines->find(s->line);
        if (it != sourceLines->end()) {
            out << indent << "/* comment: " << it->second << " */\n";
        }
    }
    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            out << indent << "ps_say(" << emitExpr(node.expr) << ");\n";
        } else if constexpr (std::is_same_v<T, SetStmt>) {
            out << indent << mangle(node.name) << " = " << emitExpr(node.expr) << ";\n";
        } else if constexpr (std::is_same_v<T, AddStmt>) {
            out << indent << mangle(node.varName) << " = ps_add(" << mangle(node.varName)
                << ", " << emitExpr(node.expr) << ");\n";
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            std::string i = "ps__i" + std::to_string(loopCounter);
            std::string n = "ps__n" + std::to_string(loopCounter);
            loopCounter++;
            out << indent << "{\n";
            out << indent << "    long " << n << " = ps_as_int(" << emitExpr(node.count) << ");\n";
            out << indent << "    for (long " << i << " = 0; " << i << " < " << n << "; " << i << "++) {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, sourceLines);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            out << indent << "if (ps_truthy(" << emitExpr(node.cond) << ")) {\n";
            for (Stmt *inner : node.thenBody) emitStmt(inner, out, indent + "    ", loopCounter, sourceLines);
            out << indent << "}";
            if (!node.elseBody.empty()) {
                out << " else {\n";
                for (Stmt *inner : node.elseBody) emitStmt(inner, out, indent + "    ", loopCounter, sourceLines);
                out << indent << "}\n";
            } else {
                out << "\n";
            }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            out << indent << "while (ps_truthy(" << emitExpr(node.cond) << ")) {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "    ", loopCounter, sourceLines);
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            out << indent << mangle(node.name) << "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) out << ", ";
                out << emitExpr(node.args[i]);
            }
            out << ");\n";
        }
    }, s->node);
}

} // namespace

void emitProcedure(const ProcedureStmt &proc, std::ostream &out,
                   const std::unordered_map<int, std::string> *sourceLines) {
    out << "void " << mangle(proc.name) << "(";
    for (size_t i = 0; i < proc.params.size(); ++i) {
        if (i > 0) out << ", ";
        out << "PsValue " << mangle(proc.params[i]);
    }
    out << ") {\n";

    std::set<std::string> localVars;
    collectVars(proc.body, localVars);
    for (const auto &v : localVars) out << "    PsValue " << mangle(v) << ";\n";
    if (!localVars.empty()) out << "\n";

    int loopCounter = 0;
    for (Stmt *inner : proc.body) emitStmt(inner, out, "    ", loopCounter, sourceLines);

    out << "}\n\n";
}

std::string emitProgram(const std::vector<Stmt *> &program,
                        const std::unordered_map<int, std::string> *sourceLines) {
    std::set<std::string> vars;
    for (Stmt *s : program) {
        std::visit([&](auto &&node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, SetStmt>) vars.insert(node.name);
            else if constexpr (std::is_same_v<T, AddStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, RepeatStmt>) collectVars(node.body, vars);
            else if constexpr (std::is_same_v<T, IfStmt>) { collectVars(node.thenBody, vars); collectVars(node.elseBody, vars); }
            else if constexpr (std::is_same_v<T, WhileStmt>) collectVars(node.body, vars);
        }, s->node);
    }

    std::ostringstream out;
    out << "/* generated by plainspeak — do not edit by hand */\n";
    out << "#include \"plainspeak_runtime.h\"\n\n";

    for (const auto &v : vars) out << "PsValue " << mangle(v) << ";\n";
    if (!vars.empty()) out << "\n";

    for (Stmt *s : program) {
        if (auto *proc = std::get_if<ProcedureStmt>(&s->node)) {
            emitProcedure(*proc, out, sourceLines);
        }
    }

    out << "int main(void) {\n";
    int loopCounter = 0;
    for (Stmt *s : program) {
        if (!std::holds_alternative<ProcedureStmt>(s->node)) {
            emitStmt(s, out, "    ", loopCounter, sourceLines);
        }
    }
    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}
