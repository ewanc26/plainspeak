#include "c_emitter.h"
#include <cstdio>
#include <set>
#include <sstream>
#include <type_traits>
#include <variant>

#include "mangling.h"

namespace {

Type exprType(const Expr *e, const AnalysisResult &analysis) {
    auto it = analysis.exprTypes.find(e);
    return it == analysis.exprTypes.end() ? Type::number() : it->second;
}

Type typeOperand(const Expr *e, const AnalysisResult &analysis) {
    auto it = analysis.typeOperands.find(e);
    return it == analysis.typeOperands.end() ? Type::number() : it->second;
}

std::string emitCBaseType(const Type &type) {
    if (type.kind == TypeKind::Boolean) return "_Bool";
    if (type.kind == TypeKind::Integer) {
        if (type.integerRank == IntegerRank::Char) {
            if (type.charSignedness == CharSignedness::Plain) return "char";
            if (type.charSignedness == CharSignedness::Unsigned) return "unsigned char";
            return "signed char";
        }
        std::string out = type.isUnsigned ? "unsigned " : "";
        switch (type.integerRank) {
            case IntegerRank::Char: return out + "char";
            case IntegerRank::Short: return out + "short";
            case IntegerRank::Int: return out + "int";
            case IntegerRank::Long: return out + "long";
            case IntegerRank::LongLong: return out + "long long";
        }
    }
    if (type.kind == TypeKind::Floating) {
        if (type.floatingRank == FloatingRank::Float) return "float";
        if (type.floatingRank == FloatingRank::LongDouble) return "long double";
        return "double";
    }
    if (type.kind == TypeKind::Void) return "void";
    if (type.kind == TypeKind::Structure) return "struct " + type.tag;
    if (type.kind == TypeKind::Union) return "union " + type.tag;
    if (type.kind == TypeKind::Enumeration) return "enum " + type.tag;
    return "long";
}

std::string emitCDeclarator(const Type &type, const std::string &name) {
    if (type.kind == TypeKind::Array && type.elementType) {
        std::string bound = type.arrayBound ? std::to_string(*type.arrayBound) : "";
        return emitCDeclarator(*type.elementType, name + "[" + bound + "]");
    }
    if (type.kind == TypeKind::Pointer && type.elementType) {
        std::string pointerName;
        if (type.elementType->kind == TypeKind::Array || type.elementType->kind == TypeKind::Function) {
            pointerName = "(*" + name + ")";
        } else {
            pointerName = "*" + name;
        }
        return emitCDeclarator(*type.elementType, pointerName);
    }
    std::string base = emitCBaseType(type);
    return name.empty() ? base : base + " " + name;
}

std::string emitCType(const Type &type) {
    const std::string marker = "__ps_type_marker";
    std::string text = emitCDeclarator(type, marker);
    std::size_t pos = text.find(marker);
    if (pos != std::string::npos) text.erase(pos, marker.size());
    return text;
}

std::string emitCDeclaration(const Type &type, const std::string &name) {
    return emitCDeclarator(type, name);
}

bool isNativeRef(const Expr *e, const AnalysisResult &analysis) {
    return analysis.nativeObjectRefs.count(e) != 0;
}

std::string emitBoxedExpr(const Expr *e, const AnalysisResult &analysis);
std::string emitRawExpr(const Expr *e, const AnalysisResult &analysis);

std::string boxRaw(const std::string &raw, const Type &type) {
    if (type.kind == TypeKind::Floating) {
        return "ps_double((double)(" + raw + "))";
    }
    if (type.kind == TypeKind::Boolean || type.kind == TypeKind::Integer ||
        type.kind == TypeKind::Enumeration || type.kind == TypeKind::BitInt) {
        return "ps_int((long)(" + raw + "))";
    }
    // Pointer values are deliberately not shoehorned into PsValue. Sema keeps
    // pointer-only operations on the raw path until the language has a native
    // pointer formatting/interop surface.
    return "ps_int(0L)";
}

std::string emitRawExpr(const Expr *e, const AnalysisResult &analysis) {
    return std::visit([&](auto &&node) -> std::string {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, AddressOfExpr>) {
            return "(&" + mangle(node.name) + ")";
        } else if constexpr (std::is_same_v<T, DerefExpr>) {
            return "(*(" + emitRawExpr(node.pointer, analysis) + "))";
        } else if constexpr (std::is_same_v<T, ElementExpr>) {
            return "((" + emitRawExpr(node.base, analysis) + ")[(" + emitRawExpr(node.index, analysis) + ")])";
        } else if constexpr (std::is_same_v<T, VarRef>) {
            if (isNativeRef(e, analysis)) return mangle(node.name);
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Type lhsType = exprType(node.lhs, analysis);
            Type rhsType = exprType(node.rhs, analysis);
            bool nativePointerOp = lhsType.isPointer() || lhsType.isArray() || rhsType.isPointer() || rhsType.isArray();
            if (nativePointerOp) {
                const char *op = node.op == BinOp::Add ? "+"
                               : node.op == BinOp::Sub ? "-"
                               : node.op == BinOp::Gt  ? ">"
                               : node.op == BinOp::Lt  ? "<"
                               : node.op == BinOp::Eq  ? "=="
                               : node.op == BinOp::Ne  ? "!="
                               : node.op == BinOp::Ge  ? ">="
                                                       : "<=";
                return "((" + emitRawExpr(node.lhs, analysis) + ") " + op + " (" +
                       emitRawExpr(node.rhs, analysis) + "))";
            }
        }

        Type type = exprType(e, analysis);
        std::string boxed = emitBoxedExpr(e, analysis);
        if (type.kind == TypeKind::Floating) {
            return "((" + emitCType(type) + ")ps_as_double(" + boxed + "))";
        }
        if (type.kind == TypeKind::Boolean || type.kind == TypeKind::Integer ||
            type.kind == TypeKind::Enumeration || type.kind == TypeKind::BitInt) {
            return "((" + emitCType(type) + ")ps_as_int(" + boxed + "))";
        }
        return boxed;
    }, e->node);
}

std::string emitBoxedExpr(const Expr *e, const AnalysisResult &analysis) {
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
            if (isNativeRef(e, analysis)) {
                return boxRaw(mangle(node.name), exprType(e, analysis));
            }
            return mangle(node.name);
        } else if constexpr (std::is_same_v<T, AddressOfExpr>) {
            return "ps_int(0L)";
        } else if constexpr (std::is_same_v<T, DerefExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, ElementExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, ListExpr>) {
            std::string result = "ps_list_from((PsValue[]){";
            for (size_t i = 0; i < node.items.size(); ++i) {
                if (i > 0) result += ", ";
                result += emitBoxedExpr(node.items[i], analysis);
            }
            result += "}, " + std::to_string(node.items.size()) + ")";
            return result;
        } else if constexpr (std::is_same_v<T, EmptyListExpr>) {
            return "ps_list_from(NULL, 0)";
        } else if constexpr (std::is_same_v<T, ItemExpr>) {
            return "ps_list_get(" + emitBoxedExpr(node.list, analysis) + ", " +
                   emitBoxedExpr(node.index, analysis) + ")";
        } else if constexpr (std::is_same_v<T, LengthExpr>) {
            return "ps_int(ps_length(" + emitBoxedExpr(node.operand, analysis) + "))";
        } else if constexpr (std::is_same_v<T, SizeOfTypeExpr> || std::is_same_v<T, SizeOfExpr>) {
            return "ps_int((long)sizeof(" + emitCType(typeOperand(e, analysis)) + "))";
        } else if constexpr (std::is_same_v<T, AlignOfTypeExpr>) {
            return "ps_int((long)_Alignof(" + emitCType(typeOperand(e, analysis)) + "))";
        } else if constexpr (std::is_same_v<T, MathCallExpr>) {
            static const std::unordered_map<std::string, std::string> mathFn = {
                {"sine", "ps_sin"}, {"cosine", "ps_cos"}, {"tangent", "ps_tan"},
                {"sqrt", "ps_sqrt"}, {"log", "ps_log"}, {"abs", "ps_abs"},
                {"floor", "ps_floor"}, {"ceil", "ps_ceil"}
            };
            auto it = mathFn.find(node.func);
            std::string fn = it != mathFn.end() ? it->second : "ps_" + node.func;
            return fn + "(" + emitBoxedExpr(node.arg, analysis) + ")";
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            static const std::unordered_map<std::string, std::string> builtins = {
                {"sin", "ps_sin"}, {"cos", "ps_cos"}, {"tan", "ps_tan"},
                {"sqrt", "ps_sqrt"}, {"log", "ps_log"}, {"abs", "ps_abs"},
                {"floor", "ps_floor"}, {"ceil", "ps_ceil"}, {"pow", "ps_pow"},
                {"neg", "ps_neg"}
            };
            auto it = builtins.find(node.name);
            std::string result = (it != builtins.end() ? it->second : mangle(node.name)) + "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) result += ", ";
                result += emitBoxedExpr(node.args[i], analysis);
            }
            result += ")";
            return result;
        } else if constexpr (std::is_same_v<T, PowExpr>) {
            return "ps_pow(" + emitBoxedExpr(node.base, analysis) + ", " +
                   emitBoxedExpr(node.exp, analysis) + ")";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Type lhsType = exprType(node.lhs, analysis);
            Type rhsType = exprType(node.rhs, analysis);
            if (lhsType.isPointer() || lhsType.isArray() || rhsType.isPointer() || rhsType.isArray()) {
                return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
            }
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
            return std::string(fn) + "(" + emitBoxedExpr(node.lhs, analysis) + ", " +
                   emitBoxedExpr(node.rhs, analysis) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            if (node.op == UnaryOp::Neg) return "ps_neg(" + emitBoxedExpr(node.rhs, analysis) + ")";
            return "ps_not(" + emitBoxedExpr(node.rhs, analysis) + ")";
        }
        return "ps_int(0L)";
    }, e->node);
}

void collectVars(const std::vector<Stmt *> &stmts, std::set<std::string> &out,
                 const AnalysisResult &analysis) {
    for (Stmt *s : stmts) {
        std::visit([&](auto &&node) {
            using T = std::decay_t<decltype(node)>;
            if constexpr (std::is_same_v<T, SetStmt>) {
                if (!analysis.nativeMutationTargets.count(s)) out.insert(node.name);
            } else if constexpr (std::is_same_v<T, AddStmt> || std::is_same_v<T, SubStmt>) {
                if (!analysis.nativeMutationTargets.count(s)) out.insert(node.varName);
            } else if constexpr (std::is_same_v<T, ReadStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReadFloatStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, AppendStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, ReplaceItemStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, RemoveItemStmt>) out.insert(node.varName);
            else if constexpr (std::is_same_v<T, RepeatStmt>) collectVars(node.body, out, analysis);
            else if constexpr (std::is_same_v<T, IfStmt>) {
                collectVars(node.thenBody, out, analysis);
                collectVars(node.elseBody, out, analysis);
            }
            else if constexpr (std::is_same_v<T, WhileStmt>) collectVars(node.body, out, analysis);
            else if constexpr (std::is_same_v<T, ForEachStmt>) collectVars(node.body, out, analysis);
        }, s->node);
    }
}

void emitStmt(const Stmt *s, std::ostream &out, const std::string &indent,
              int &loopCounter, const AnalysisResult &analysis,
              const std::unordered_map<int, std::string> *sourceLines) {
    bool isComment = std::holds_alternative<CommentStmt>(s->node);
    if (sourceLines && !isComment) {
        auto it = sourceLines->find(s->line);
        if (it != sourceLines->end()) out << indent << "/* from source: " << it->second << " */\n";
    }

    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            out << indent << "ps_say(" << emitBoxedExpr(node.expr, analysis) << ");\n";
        } else if constexpr (std::is_same_v<T, SetStmt>) {
            if (analysis.nativeMutationTargets.count(s)) {
                out << indent << mangle(node.name) << " = " << emitRawExpr(node.expr, analysis) << ";\n";
            } else {
                out << indent << mangle(node.name) << " = " << emitBoxedExpr(node.expr, analysis) << ";\n";
            }
        } else if constexpr (std::is_same_v<T, NativeDeclStmt>) {
            Type type = analysis.declarationTypes.at(s);
            out << indent << emitCDeclaration(type, mangle(node.name));
            if (node.initializer) out << " = " << emitRawExpr(node.initializer, analysis);
            out << ";\n";
        } else if constexpr (std::is_same_v<T, StoreThroughStmt>) {
            out << indent << "*(" << emitRawExpr(node.pointer, analysis) << ") = "
                << emitRawExpr(node.expr, analysis) << ";\n";
        } else if constexpr (std::is_same_v<T, StoreElementStmt>) {
            out << indent << "(" << emitRawExpr(node.base, analysis) << ")[("
                << emitRawExpr(node.index, analysis) << ")] = "
                << emitRawExpr(node.expr, analysis) << ";\n";
        } else if constexpr (std::is_same_v<T, AddStmt>) {
            if (analysis.nativeMutationTargets.count(s)) {
                out << indent << mangle(node.varName) << " += " << emitRawExpr(node.expr, analysis) << ";\n";
            } else {
                out << indent << mangle(node.varName) << " = ps_add(" << mangle(node.varName)
                    << ", " << emitBoxedExpr(node.expr, analysis) << ");\n";
            }
        } else if constexpr (std::is_same_v<T, SubStmt>) {
            if (analysis.nativeMutationTargets.count(s)) {
                out << indent << mangle(node.varName) << " -= " << emitRawExpr(node.expr, analysis) << ";\n";
            } else {
                out << indent << mangle(node.varName) << " = ps_sub(" << mangle(node.varName)
                    << ", " << emitBoxedExpr(node.expr, analysis) << ");\n";
            }
        } else if constexpr (std::is_same_v<T, ReadStmt>) {
            out << indent << mangle(node.varName) << " = ps_read();\n";
        } else if constexpr (std::is_same_v<T, ReadFloatStmt>) {
            out << indent << mangle(node.varName) << " = ps_read_double();\n";
        } else if constexpr (std::is_same_v<T, AppendStmt>) {
            out << indent << "ps_list_append(" << mangle(node.varName) << ", "
                << emitBoxedExpr(node.expr, analysis) << ");\n";
        } else if constexpr (std::is_same_v<T, ReplaceItemStmt>) {
            out << indent << "ps_list_set(" << mangle(node.varName) << ", "
                << emitBoxedExpr(node.index, analysis) << ", "
                << emitBoxedExpr(node.expr, analysis) << ");\n";
        } else if constexpr (std::is_same_v<T, RemoveItemStmt>) {
            out << indent << "ps_list_remove(" << mangle(node.varName) << ", "
                << emitBoxedExpr(node.index, analysis) << ");\n";
        } else if constexpr (std::is_same_v<T, CommentStmt>) {
            out << indent << "/* " << node.text << " */\n";
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            int id = loopCounter++;
            std::string i = "ps__i" + std::to_string(id);
            std::string n = "ps__n" + std::to_string(id);
            out << indent << "{\n";
            out << indent << "    long " << n << " = ps_as_int(" << emitBoxedExpr(node.count, analysis) << ");\n";
            out << indent << "    for (long " << i << " = 0; " << i << " < " << n << "; " << i << "++) {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, analysis, sourceLines);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            out << indent << "if (ps_truthy(" << emitBoxedExpr(node.cond, analysis) << ")) {\n";
            for (Stmt *inner : node.thenBody) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines);
            out << indent << "}";
            if (!node.elseBody.empty()) {
                out << " else {\n";
                for (Stmt *inner : node.elseBody) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines);
                out << indent << "}\n";
            } else {
                out << "\n";
            }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            out << indent << "while (ps_truthy(" << emitBoxedExpr(node.cond, analysis) << ")) {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines);
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            int id = loopCounter++;
            std::string list = "ps__list" + std::to_string(id);
            std::string i = "ps__i" + std::to_string(id);
            std::string n = "ps__n" + std::to_string(id);
            out << indent << "{\n";
            out << indent << "    PsValue " << list << " = ps_list_copy(" << emitBoxedExpr(node.list, analysis) << ");\n";
            out << indent << "    long " << n << " = ps_length(" << list << ");\n";
            out << indent << "    for (long " << i << " = 1; " << i << " <= " << n << "; " << i << "++) {\n";
            out << indent << "        PsValue " << mangle(node.itemName) << " = ps_list_get(" << list << ", ps_int(" << i << "));\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, analysis, sourceLines);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            out << indent << "(void)" << mangle(node.name) << "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) out << ", ";
                out << emitBoxedExpr(node.args[i], analysis);
            }
            out << ");\n";
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            out << indent << "return " << emitBoxedExpr(node.expr, analysis) << ";\n";
        }
    }, s->node);
}

void emitProcedure(const ProcedureStmt &proc, std::ostream &out,
                   const AnalysisResult &analysis,
                   const std::unordered_map<int, std::string> *sourceLines) {
    out << "PsValue " << mangle(proc.name) << "(";
    for (size_t i = 0; i < proc.params.size(); ++i) {
        if (i > 0) out << ", ";
        out << "PsValue " << mangle(proc.params[i]);
    }
    out << ") {\n";

    std::set<std::string> localVars;
    collectVars(proc.body, localVars, analysis);
    for (const auto &v : localVars) out << "    PsValue " << mangle(v) << ";\n";
    if (!localVars.empty()) out << "\n";

    int loopCounter = 0;
    for (Stmt *inner : proc.body) emitStmt(inner, out, "    ", loopCounter, analysis, sourceLines);

    out << "    return ps_int(0L);\n";
    out << "}\n\n";
}

} // namespace

std::string emitProgram(const std::vector<Stmt *> &program,
                        const AnalysisResult &analysis,
                        const std::unordered_map<int, std::string> *sourceLines) {
    std::set<std::string> vars;
    collectVars(program, vars, analysis);

    std::ostringstream out;
    out << "/* generated by plainspeak — do not edit by hand */\n";
    out << "#include \"plainspeak_runtime.h\"\n\n";

    for (const auto &v : vars) out << "PsValue " << mangle(v) << ";\n";

    // Direct top-level native declarations are real file-scope C objects so
    // procedures can take their address or access them. Their possibly-dynamic
    // PlainSpeak initializers are executed in main below.
    for (Stmt *s : program) {
        if (auto *decl = std::get_if<NativeDeclStmt>(&s->node)) {
            out << emitCDeclaration(analysis.declarationTypes.at(s), mangle(decl->name)) << ";\n";
        }
    }
    if (!vars.empty()) out << "\n";

    for (Stmt *s : program) {
        if (auto *proc = std::get_if<ProcedureStmt>(&s->node)) {
            emitProcedure(*proc, out, analysis, sourceLines);
        }
    }

    out << "int main(void) {\n";
    int loopCounter = 0;
    for (Stmt *s : program) {
        if (std::holds_alternative<ProcedureStmt>(s->node)) continue;
        if (auto *decl = std::get_if<NativeDeclStmt>(&s->node)) {
            if (decl->initializer) {
                out << "    " << mangle(decl->name) << " = "
                    << emitRawExpr(decl->initializer, analysis) << ";\n";
            }
            continue;
        }
        emitStmt(s, out, "    ", loopCounter, analysis, sourceLines);
    }
    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}
