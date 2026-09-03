#include "c_emitter.h"
#include <cstdio>
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
            else if constexpr (std::is_same_v<T, SubStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReadStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReadFloatStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, AppendStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReplaceItemStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, RemoveItemStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, RepeatStmt>) collectVars(node.body, out);
            else if constexpr (std::is_same_v<T, IfStmt>) { collectVars(node.thenBody, out); collectVars(node.elseBody, out); }
            else if constexpr (std::is_same_v<T, WhileStmt>) collectVars(node.body, out);
            else if constexpr (std::is_same_v<T, ForEachStmt>) collectVars(node.body, out);
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
        } else if constexpr (std::is_same_v<T, FloatLit>) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.17g", node.value);
            return "ps_double(" + std::string(buf) + ")";
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
        } else if constexpr (std::is_same_v<T, ListExpr>) {
            std::string result = "ps_list_from((PsValue[]){";
            for (size_t i = 0; i < node.items.size(); ++i) {
                if (i > 0) result += ", ";
                result += emitExpr(node.items[i]);
            }
            result += "}, " + std::to_string(node.items.size()) + ")";
            return result;
        } else if constexpr (std::is_same_v<T, EmptyListExpr>) {
            return "ps_list_from(NULL, 0)";
        } else if constexpr (std::is_same_v<T, ItemExpr>) {
            return "ps_list_get(" + emitExpr(node.list) + ", " + emitExpr(node.index) + ")";
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            return "ps_int(ps_length(" + emitExpr(node.operand) + "))";
        } else if constexpr (std::is_same_v<T, MathCallExpr>) {
            static const std::unordered_map<std::string, std::string> mathFn = {
                {"sine", "ps_sin"}, {"cosine", "ps_cos"}, {"tangent", "ps_tan"},
                {"sqrt", "ps_sqrt"}, {"log", "ps_log"}, {"abs", "ps_abs"},
                {"floor", "ps_floor"}, {"ceil", "ps_ceil"}
            };
            auto it = mathFn.find(node.func);
            std::string fn = it != mathFn.end() ? it->second : "ps_" + node.func;
            return fn + "(" + emitExpr(node.arg) + ")";
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            static const std::unordered_map<std::string, std::string> builtins = {
                {"sin", "ps_sin"}, {"cos", "ps_cos"}, {"tan", "ps_tan"},
                {"sqrt", "ps_sqrt"}, {"log", "ps_log"}, {"abs", "ps_abs"},
                {"floor", "ps_floor"}, {"ceil", "ps_ceil"}, {"pow", "ps_pow"},
                {"neg", "ps_neg"}
            };
            auto it = builtins.find(node.name);
            if (it != builtins.end()) {
                std::string result = it->second + "(";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += emitExpr(node.args[i]);
                }
                result += ")";
                return result;
            }
            std::string result = mangle(node.name) + "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) result += ", ";
                result += emitExpr(node.args[i]);
            }
            result += ")";
            return result;
        } else if constexpr (std::is_same_v<T, PowExpr>) {
            return "ps_pow(" + emitExpr(node.base) + ", " + emitExpr(node.exp) + ")";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            const char *fn = node.op == BinOp::Add ? "ps_add"
                            : node.op == BinOp::Sub ? "ps_sub"
                            : node.op == BinOp::Mul ? "ps_mul"
                            : node.op == BinOp::Div ? "ps_div"
                            : node.op == BinOp::Mod ? "ps_mod"
                            : node.op == BinOp::Gt  ? "ps_gt"
                            : node.op == BinOp::Lt  ? "ps_lt"
                            : node.op == BinOp::Eq  ? "ps_eq"
                            : node.op == BinOp::Ne  ? "ps_ne"
                            : node.op == BinOp::Ge  ? "ps_ge"
                            : node.op == BinOp::Le  ? "ps_le"
                            : node.op == BinOp::And ? "ps_and"
                                                     : "ps_or";
            return std::string(fn) + "(" + emitExpr(node.lhs) + ", " + emitExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.op == UnaryOp::Neg) {
                return std::string("ps_neg(") + emitExpr(node.rhs) + ")";
            }
            return std::string("ps_not(") + emitExpr(node.rhs) + ")";
        }
        return "ps_int(0L)";
    }, e->node);
}

void emitStmt(const Stmt *s, std::ostream &out, std::string indent, int &loopCounter,
              const std::unordered_map<int, std::string> *sourceLines) {
    bool isComment = std::holds_alternative<CommentStmt>(s->node);
    if (sourceLines && !isComment) {
        auto it = sourceLines->find(s->line);
        if (it != sourceLines->end()) {
            out << indent << "/* from source: " << it->second << " */\n";
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
        } else if constexpr (std::is_same_v<T, SubStmt>) {
            out << indent << mangle(node.varName) << " = ps_sub(" << mangle(node.varName)
                << ", " << emitExpr(node.expr) << ");\n";
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            out << indent << mangle(node.varName) << " = ps_read();\n";
        } else if constexpr (std::is_same_v<T, ReadFloatStmt>) {
            out << indent << mangle(node.varName) << " = ps_read_double();\n";
        } else if constexpr (std::is_same_v<T, AppendStmt>) {
            out << indent << "ps_list_append(" << mangle(node.varName) << ", " << emitExpr(node.expr) << ");\n";
        } else if constexpr (std::is_same_v<T, ReplaceItemStmt>) {
            out << indent << "ps_list_set(" << mangle(node.varName) << ", " << emitExpr(node.index)
                << ", " << emitExpr(node.expr) << ");\n";
        } else if constexpr (std::is_same_v<T, RemoveItemStmt>) {
            out << indent << "ps_list_remove(" << mangle(node.varName) << ", " << emitExpr(node.index) << ");\n";
        } else if constexpr (std::is_same_v<T, CommentStmt>) {
            out << indent << "/* " << node.text << " */\n";
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            int id = loopCounter++;
            std::string i = "ps__i" + std::to_string(id);
            std::string n = "ps__n" + std::to_string(id);
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
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            int id = loopCounter++;
            std::string list = "ps__list" + std::to_string(id);
            std::string i = "ps__i" + std::to_string(id);
            std::string n = "ps__n" + std::to_string(id);
            out << indent << "{\n";
            out << indent << "    PsValue " << list << " = ps_list_copy(" << emitExpr(node.list) << ");\n";
            out << indent << "    long " << n << " = ps_length(" << list << ");\n";
            out << indent << "    for (long " << i << " = 1; " << i << " <= " << n << "; " << i << "++) {\n";
            out << indent << "        PsValue " << mangle(node.itemName) << " = ps_list_get(" << list << ", ps_int(" << i << "));\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, sourceLines);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            out << indent << "(void)" << mangle(node.name) << "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) out << ", ";
                out << emitExpr(node.args[i]);
            }
            out << ");\n";
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            out << indent << "return " << emitExpr(node.expr) << ";\n";
        }
    }, s->node);
}

} // namespace

void emitProcedure(const ProcedureStmt &proc, std::ostream &out,
                   const std::unordered_map<int, std::string> *sourceLines) {
    out << "PsValue " << mangle(proc.name) << "(";
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

    out << "    return ps_int(0L);\n";
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
            else if constexpr (std::is_same_v<T, SubStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReadStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReadFloatStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, AppendStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReplaceItemStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, RemoveItemStmt>) vars.insert(node.varName);
            else if constexpr (std::is_same_v<T, RepeatStmt>) collectVars(node.body, vars);
            else if constexpr (std::is_same_v<T, IfStmt>) { collectVars(node.thenBody, vars); collectVars(node.elseBody, vars); }
            else if constexpr (std::is_same_v<T, WhileStmt>) collectVars(node.body, vars);
            else if constexpr (std::is_same_v<T, ForEachStmt>) collectVars(node.body, vars);
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
