#include "c_emitter.h"
#include <algorithm>
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

std::string emitQualifierWords(const TypeQualifiers &q) {
    std::string out;
    auto add = [&](const char *word) {
        if (!out.empty()) out += " ";
        out += word;
    };
    if (q.isConst) add("const");
    if (q.isVolatile) add("volatile");
    if (q.isRestrict) add("restrict");
    if (q.isAtomic) add("_Atomic");
    return out;
}

std::string emitCUnqualifiedBaseType(const Type &type) {
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
    if (type.kind == TypeKind::BitInt) {
        return std::string(type.isUnsigned ? "unsigned _BitInt(" : "_BitInt(") + std::to_string(type.bitWidth) + ")";
    }
    if (type.kind == TypeKind::Floating) {
        if (type.floatingRank == FloatingRank::Float) return "float";
        if (type.floatingRank == FloatingRank::LongDouble) return "long double";
        return "double";
    }
    if (type.kind == TypeKind::Complex) return "double _Complex";
    if (type.kind == TypeKind::Void) return "void";
    if (type.kind == TypeKind::Structure) return "struct " + mangle(type.tag);
    if (type.kind == TypeKind::Union) return "union " + mangle(type.tag);
    if (type.kind == TypeKind::Enumeration) return "enum " + mangle(type.tag);
    if (type.kind == TypeKind::Nullptr) return "PsNullptr";
    return "long";
}

std::string emitCBaseType(const Type &type) {
    Type unqualified = type;
    unqualified.qualifiers = {};
    std::string base = emitCUnqualifiedBaseType(unqualified);
    std::string qualifiers = emitQualifierWords(type.qualifiers);
    return qualifiers.empty() ? base : qualifiers + " " + base;
}

std::string emitCDeclarator(const Type &type, const std::string &name) {
    if (type.kind == TypeKind::Array && type.elementType) {
        std::string bound = type.arrayBound ? std::to_string(*type.arrayBound) : "";
        return emitCDeclarator(*type.elementType, name + "[" + bound + "]");
    }
    if (type.kind == TypeKind::Pointer && type.elementType) {
        std::string pointerPart = "*";
        std::string qualifiers = emitQualifierWords(type.qualifiers);
        if (!qualifiers.empty()) pointerPart += " " + qualifiers;
        if (!name.empty()) {
            if (!qualifiers.empty()) pointerPart += " ";
            pointerPart += name;
        }

        std::string pointerName;
        if (type.elementType->kind == TypeKind::Array || type.elementType->kind == TypeKind::Function) {
            pointerName = "(" + pointerPart + ")";
        } else {
            pointerName = pointerPart;
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

std::string emitAggregateFieldDeclaration(const AggregateFieldInfo &field) {
    const std::string name = field.name.empty() ? std::string() : mangle(field.name);
    std::string declaration = emitCDeclaration(field.type, name);
    if (field.bitWidth) declaration += " : " + std::to_string(*field.bitWidth);
    return declaration;
}

std::string mangleEnumerator(const std::string &enumeration, const std::string &name) {
    return mangle(enumeration) + "__" + mangle(name);
}

std::string emitBoxedExpr(const Expr *e, const AnalysisResult &analysis);
std::string emitRawExpr(const Expr *e, const AnalysisResult &analysis);

void emitAggregateStores(const std::string &name, const Type &declared,
                         const AggregateInitializer &aggregate,
                         std::ostream &out, const std::string &indent,
                         const AnalysisResult &analysis) {
    auto emitStore = [&](const std::string &target, Expr *expr) {
        out << indent << target << " = " << emitRawExpr(expr, analysis) << ";\n";
    };

    if (aggregate.kind == AggregateInitKind::Positional) {
        if (declared.isArray()) {
            for (std::size_t i = 0; i < aggregate.entries.size(); ++i) {
                emitStore(name + "[" + std::to_string(i) + "]", aggregate.entries[i].expr);
            }
            return;
        }

        const StructureInfo *info = nullptr;
        if (declared.kind == TypeKind::Structure) {
            auto it = analysis.structures.find(declared.tag);
            if (it != analysis.structures.end()) info = &it->second;
        } else if (declared.kind == TypeKind::Union) {
            auto it = analysis.unions.find(declared.tag);
            if (it != analysis.unions.end()) info = &it->second;
        }
        if (!info) return;
        std::vector<const AggregateFieldInfo *> positionalFields;
        for (const auto &field : info->fields) {
            if (!field.name.empty() && !field.flexibleArray) positionalFields.push_back(&field);
        }
        std::size_t count = std::min(aggregate.entries.size(), positionalFields.size());
        if (declared.kind == TypeKind::Union) count = std::min<std::size_t>(count, 1);
        for (std::size_t i = 0; i < count; ++i) {
            emitStore("(" + name + ")." + mangle(positionalFields[i]->name), aggregate.entries[i].expr);
        }
        return;
    }

    if (aggregate.kind == AggregateInitKind::Members) {
        for (const auto &entry : aggregate.entries) {
            emitStore("(" + name + ")." + mangle(entry.memberName), entry.expr);
        }
        return;
    }

    for (const auto &entry : aggregate.entries) {
        emitStore(name + "[" + std::to_string(entry.elementIndex) + "]", entry.expr);
    }
}

bool isNativeRef(const Expr *e, const AnalysisResult &analysis) {
    return analysis.nativeObjectRefs.count(e) != 0;
}

const ProcedureSignature *procedureSignature(const std::string &name,
                                             const AnalysisResult &analysis) {
    auto it = analysis.procedureSignatures.find(name);
    return it == analysis.procedureSignatures.end() ? nullptr : &it->second;
}

std::string emitBoxedExpr(const Expr *e, const AnalysisResult &analysis);
std::string emitRawExpr(const Expr *e, const AnalysisResult &analysis);

bool isCArithmeticType(const Type &type) {
    return type.kind == TypeKind::Boolean || type.kind == TypeKind::Integer ||
           type.kind == TypeKind::Floating || type.kind == TypeKind::Enumeration ||
           type.kind == TypeKind::BitInt || type.kind == TypeKind::Complex;
}

bool isCScalarType(const Type &type) {
    return isCArithmeticType(type) || type.isPointer() || type.kind == TypeKind::Nullptr;
}

const char *emitBinaryOperator(BinOp op) {
    switch (op) {
        case BinOp::Add: return "+";
        case BinOp::Sub: return "-";
        case BinOp::Mul: return "*";
        case BinOp::Div: return "/";
        case BinOp::Mod: return "%";
        case BinOp::ShiftLeft: return "<<";
        case BinOp::ShiftRight: return ">>";
        case BinOp::Gt: return ">";
        case BinOp::Lt: return "<";
        case BinOp::Eq: return "==";
        case BinOp::Ne: return "!=";
        case BinOp::Ge: return ">=";
        case BinOp::Le: return "<=";
        case BinOp::BitAnd: return "&";
        case BinOp::BitXor: return "^";
        case BinOp::BitOr: return "|";
        case BinOp::And: return "&&";
        case BinOp::Or: return "||";
    }
    return "?";
}

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
        if constexpr (std::is_same_v<T, IntLit>) {
            return std::to_string(node.value) + "L";
        } else if constexpr (std::is_same_v<T, BoolLit>) {
            return node.value ? "1" : "0";
        } else if constexpr (std::is_same_v<T, FloatLit>) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%.17g", node.value);
            std::string literal(buf);
            if (literal.find_first_of(".eE") == std::string::npos) literal += ".0";
            return literal;
        } else if constexpr (std::is_same_v<T, NullptrLit>) {
            // Keep the predefined null literal as an integer null pointer
            // constant on the C11 backend. Stored nullptr_t objects use
            // PsNullptr; the literal itself must retain null-constant behavior
            // in pointer conversions and conditional expressions.
            return "0";
        } else if constexpr (std::is_same_v<T, AddressOfExpr>) {
            return "(&" + mangle(node.name) + ")";
        } else if constexpr (std::is_same_v<T, DerefExpr>) {
            return "(*(" + emitRawExpr(node.pointer, analysis) + "))";
        } else if constexpr (std::is_same_v<T, CastExpr>) {
            return "((" + emitCType(exprType(e, analysis)) + ")(" +
                   emitRawExpr(node.operand, analysis) + "))";
        } else if constexpr (std::is_same_v<T, IncDecExpr>) {
            const bool increment = node.kind == IncDecKind::PrefixIncrement ||
                                   node.kind == IncDecKind::PostfixIncrement;
            const bool prefix = node.kind == IncDecKind::PrefixIncrement ||
                                node.kind == IncDecKind::PrefixDecrement;
            std::string operand = "(" + emitRawExpr(node.operand, analysis) + ")";
            if (prefix) return "(" + std::string(increment ? "++" : "--") + operand + ")";
            return "(" + operand + std::string(increment ? "++" : "--") + ")";
        } else if constexpr (std::is_same_v<T, ConditionalExpr>) {
            std::string conditional = "((" + emitRawExpr(node.condition, analysis) + ") ? (" +
                                      emitRawExpr(node.whenTrue, analysis) + ") : (" +
                                      emitRawExpr(node.whenFalse, analysis) + "))";
            // Two source nullptr_t branches have nullptr_t result type in C23.
            // Keep bare zero for pointer-context null constants, but preserve
            // the result type when the conditional itself is nullptr_t.
            Type resultType = exprType(e, analysis);
            if (resultType.kind == TypeKind::Nullptr) {
                return "((PsNullptr)(" + conditional + "))";
            }
            if (resultType.isPointer() &&
                (exprType(node.whenTrue, analysis).kind == TypeKind::Nullptr ||
                 exprType(node.whenFalse, analysis).kind == TypeKind::Nullptr)) {
                return "((" + emitCType(resultType) + ")(" + conditional + "))";
            }
            return conditional;
        } else if constexpr (std::is_same_v<T, ElementExpr>) {
            return "((" + emitRawExpr(node.base, analysis) + ")[(" + emitRawExpr(node.index, analysis) + ")])";
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            Type baseType = exprType(node.base, analysis);
            std::string op = baseType.isPointer() ? "->" : ".";
            return "((" + emitRawExpr(node.base, analysis) + ")" + op + mangle(node.name) + ")";
        } else if constexpr (std::is_same_v<T, EnumeratorExpr>) {
            return mangleEnumerator(node.enumeration, node.name);
        } else if constexpr (std::is_same_v<T, VarRef>) {
            if (isNativeRef(e, analysis)) return mangle(node.name);
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            const ProcedureSignature *signature = procedureSignature(node.name, analysis);
            if (signature && signature->nativeTyped) {
                std::string result = mangle(node.name) + "(";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += emitRawExpr(node.args[i], analysis);
                }
                return result + ")";
            }
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            Type lhsType = exprType(node.lhs, analysis);
            Type rhsType = exprType(node.rhs, analysis);
            bool nativePointerOp = lhsType.isPointer() || lhsType.isArray() || rhsType.isPointer() || rhsType.isArray();
            if (nativePointerOp) {
                return "((" + emitRawExpr(node.lhs, analysis) + ") " + emitBinaryOperator(node.op) + " (" +
                       emitRawExpr(node.rhs, analysis) + "))";
            }
            if ((node.op == BinOp::Eq || node.op == BinOp::Ne) &&
                (lhsType.kind == TypeKind::Nullptr || rhsType.kind == TypeKind::Nullptr)) {
                return "((" + emitRawExpr(node.lhs, analysis) + ") " + emitBinaryOperator(node.op) + " (" +
                       emitRawExpr(node.rhs, analysis) + "))";
            }
            if ((node.op == BinOp::And || node.op == BinOp::Or) &&
                isCScalarType(lhsType) && isCScalarType(rhsType)) {
                return "((" + emitRawExpr(node.lhs, analysis) + ") " + emitBinaryOperator(node.op) + " (" +
                       emitRawExpr(node.rhs, analysis) + "))";
            }
            if (isCArithmeticType(lhsType) && isCArithmeticType(rhsType) &&
                node.op != BinOp::And && node.op != BinOp::Or) {
                return "((" + emitRawExpr(node.lhs, analysis) + ") " + emitBinaryOperator(node.op) + " (" +
                       emitRawExpr(node.rhs, analysis) + "))";
            }
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Type rhsType = exprType(node.rhs, analysis);
            if (node.op == UnaryOp::Not && isCScalarType(rhsType)) {
                return "(!(" + emitRawExpr(node.rhs, analysis) + "))";
            }
            if (isCArithmeticType(rhsType)) {
                const char *op = node.op == UnaryOp::Neg ? "-" :
                                 node.op == UnaryOp::BitNot ? "~" : "!";
                return "(" + std::string(op) + "(" + emitRawExpr(node.rhs, analysis) + "))";
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
        } else if constexpr (std::is_same_v<T, NullptrLit>) {
            return "ps_int(0L)";
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
        } else if constexpr (std::is_same_v<T, CastExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, IncDecExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, ConditionalExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, ElementExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
        } else if constexpr (std::is_same_v<T, EnumeratorExpr>) {
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
            if (node.func == "real") return "ps_double(creal(" + emitRawExpr(node.arg, analysis) + "))";
            if (node.func == "imaginary") return "ps_double(cimag(" + emitRawExpr(node.arg, analysis) + "))";
            if (node.func == "magnitude") return "ps_double(cabs(" + emitRawExpr(node.arg, analysis) + "))";
            if (node.func == "conjugate") return "conj(" + emitRawExpr(node.arg, analysis) + ")";
            static const std::unordered_set<std::string> ctypeFns = {
                "isalpha", "isalnum", "isblank", "iscntrl", "isdigit", "isgraph",
                "islower", "isprint", "ispunct", "isspace", "isupper", "isxdigit"
            };
            if (ctypeFns.count(node.func)) {
                return "ps_int((long)" + node.func + "((unsigned char)ps_as_int(" +
                       emitBoxedExpr(node.arg, analysis) + ")))";
            }
            auto it = mathFn.find(node.func);
            std::string fn = it != mathFn.end() ? it->second : "ps_" + node.func;
            return fn + "(" + emitBoxedExpr(node.arg, analysis) + ")";
        } else if constexpr (std::is_same_v<T, CallExpr>) {
            const ProcedureSignature *signature = procedureSignature(node.name, analysis);
            if (signature && signature->nativeTyped) {
                std::string raw = mangle(node.name) + "(";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i > 0) raw += ", ";
                    raw += emitRawExpr(node.args[i], analysis);
                }
                raw += ")";
                return boxRaw(raw, signature->returnType);
            }

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
            if (lhsType.isPointer() || lhsType.isArray() || rhsType.isPointer() || rhsType.isArray() ||
                lhsType.kind == TypeKind::Nullptr || rhsType.kind == TypeKind::Nullptr ||
                (isCArithmeticType(lhsType) && isCArithmeticType(rhsType) &&
                 node.op != BinOp::And && node.op != BinOp::Or)) {
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
            if (node.op == UnaryOp::Not && isCScalarType(exprType(node.rhs, analysis))) {
                return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
            }
            if (node.op == UnaryOp::BitNot && isCArithmeticType(exprType(node.rhs, analysis))) {
                return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
            }
            if (node.op == UnaryOp::Neg && isCArithmeticType(exprType(node.rhs, analysis))) {
                return boxRaw(emitRawExpr(e, analysis), exprType(e, analysis));
            }
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
            else if constexpr (std::is_same_v<T, DoWhileStmt>) collectVars(node.body, out, analysis);
            else if constexpr (std::is_same_v<T, ForEachStmt>) collectVars(node.body, out, analysis);
            else if constexpr (std::is_same_v<T, ForStmt>) collectVars(node.body, out, analysis);
            else if constexpr (std::is_same_v<T, SwitchStmt>) {
                for (const auto &c : node.cases) collectVars(c.body, out, analysis);
            }
        }, s->node);
    }
}

void emitStmt(const Stmt *s, std::ostream &out, const std::string &indent,
              int &loopCounter, const AnalysisResult &analysis,
              const std::unordered_map<int, std::string> *sourceLines,
              const ProcedureSignature *currentProcedure = nullptr) {
    bool isComment = std::holds_alternative<CommentStmt>(s->node);
    if (sourceLines && !isComment) {
        auto it = sourceLines->find(s->line);
        if (it != sourceLines->end()) out << indent << "/* from source: " << it->second << " */\n";
    }

    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            if (node.args.size() == 1) {
                out << indent << "ps_say(" << emitBoxedExpr(node.args[0], analysis) << ");\n";
            } else {
                out << indent << "ps_say_many(" << node.args.size() << ", (PsValue[]){";
                for (size_t i = 0; i < node.args.size(); ++i) {
                    if (i > 0) out << ", ";
                    out << emitBoxedExpr(node.args[i], analysis);
                }
                out << "});\n";
            }
        } else if constexpr (std::is_same_v<T, StaticAssertStmt>) {
            out << indent << "_Static_assert(" << emitRawExpr(node.condition, analysis) << ", \"PlainSpeak static assertion\");\n";
        } else if constexpr (std::is_same_v<T, RuntimeAssertStmt>) {
            out << indent << "assert(" << emitRawExpr(node.condition, analysis) << ");\n";
        } else if constexpr (std::is_same_v<T, AtomicFenceStmt>) {
            out << indent << "atomic_thread_fence(memory_order_seq_cst);\n";
        } else if constexpr (std::is_same_v<T, SetStmt>) {
            if (analysis.nativeMutationTargets.count(s)) {
                out << indent << mangle(node.name) << " = " << emitRawExpr(node.expr, analysis) << ";\n";
            } else {
                out << indent << mangle(node.name) << " = " << emitBoxedExpr(node.expr, analysis) << ";\n";
            }
        } else if constexpr (std::is_same_v<T, StructureStmt> ||
                             std::is_same_v<T, UnionStmt> ||
                             std::is_same_v<T, EnumerationStmt>) {
            // Top-level native type definitions are emitted before declarations
            // and procedure prototypes by emitProgram.
        } else if constexpr (std::is_same_v<T, NativeDeclStmt>) {
            Type type = analysis.declarationTypes.at(s);
            if (node.alignment) out << indent << "_Alignas(" << *node.alignment << ") ";
            else out << indent;
            out << (node.threadLocal ? "_Thread_local " : "")
                << (node.constexprObject ? "const " : "")
                << emitCDeclaration(type, mangle(node.name));
            if (node.initializer) {
                out << " = " << emitRawExpr(node.initializer, analysis);
            } else if (node.aggregateInitializer) {
                out << " = {0}";
            } else if (type.kind == TypeKind::Nullptr) {
                // C23 default initialization of nullptr_t is initialization by
                // nullptr. Static objects are already zero-initialized; this
                // handles automatic declarations.
                out << " = 0";
            }
            out << ";\n";
            if (node.aggregateInitializer) {
                emitAggregateStores(mangle(node.name), type, *node.aggregateInitializer,
                                    out, indent, analysis);
            }
        } else if constexpr (std::is_same_v<T, StoreThroughStmt>) {
            out << indent << "*(" << emitRawExpr(node.pointer, analysis) << ") = "
                << emitRawExpr(node.expr, analysis) << ";\n";
        } else if constexpr (std::is_same_v<T, StoreElementStmt>) {
            out << indent << "(" << emitRawExpr(node.base, analysis) << ")[("
                << emitRawExpr(node.index, analysis) << ")] = "
                << emitRawExpr(node.expr, analysis) << ";\n";
        } else if constexpr (std::is_same_v<T, StoreMemberStmt>) {
            Type baseType = exprType(node.base, analysis);
            out << indent << "(" << emitRawExpr(node.base, analysis) << ")"
                << (baseType.isPointer() ? "->" : ".") << mangle(node.name)
                << " = " << emitRawExpr(node.expr, analysis) << ";\n";
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
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
            out << indent << "break;\n";
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            out << indent << "continue;\n";
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            int id = loopCounter++;
            std::string i = "ps__i" + std::to_string(id);
            std::string n = "ps__n" + std::to_string(id);
            out << indent << "{\n";
            out << indent << "    long " << n << " = ps_as_int(" << emitBoxedExpr(node.count, analysis) << ");\n";
            out << indent << "    for (long " << i << " = 0; " << i << " < " << n << "; " << i << "++) {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, analysis, sourceLines, currentProcedure);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            Type condType = exprType(node.cond, analysis);
            if (isCScalarType(condType)) {
                out << indent << "if (" << emitRawExpr(node.cond, analysis) << ") {\n";
            } else {
                out << indent << "if (ps_truthy(" << emitBoxedExpr(node.cond, analysis) << ")) {\n";
            }
            for (Stmt *inner : node.thenBody) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines, currentProcedure);
            out << indent << "}";
            if (!node.elseBody.empty()) {
                out << " else {\n";
                for (Stmt *inner : node.elseBody) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines, currentProcedure);
                out << indent << "}\n";
            } else {
                out << "\n";
            }
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            Type condType = exprType(node.cond, analysis);
            if (isCScalarType(condType)) {
                out << indent << "while (" << emitRawExpr(node.cond, analysis) << ") {\n";
            } else {
                out << indent << "while (ps_truthy(" << emitBoxedExpr(node.cond, analysis) << ")) {\n";
            }
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines, currentProcedure);
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            out << indent << "do {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines, currentProcedure);
            Type condType = exprType(node.cond, analysis);
            if (isCScalarType(condType)) {
                out << indent << "} while (" << emitRawExpr(node.cond, analysis) << ");\n";
            } else {
                out << indent << "} while (ps_truthy(" << emitBoxedExpr(node.cond, analysis) << "));\n";
            }
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
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, analysis, sourceLines, currentProcedure);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            int id = loopCounter++;
            std::string from = "ps__from" + std::to_string(id);
            std::string to = "ps__to" + std::to_string(id);
            out << indent << "{\n";
            out << indent << "    long " << from << " = ps_as_int(" << emitBoxedExpr(node.from, analysis) << ");\n";
            out << indent << "    long " << to << " = ps_as_int(" << emitBoxedExpr(node.to, analysis) << ");\n";
            out << indent << "    for (long " << mangle(node.varName) << " = " << from << "; "
                << mangle(node.varName) << " " << (node.descending ? ">=" : "<=") << " " << to << "; "
                << mangle(node.varName) << (node.descending ? "--" : "++") << ") {\n";
            for (Stmt *inner : node.body) emitStmt(inner, out, indent + "        ", loopCounter, analysis, sourceLines, currentProcedure);
            out << indent << "    }\n";
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            out << indent << "switch (ps_as_int(" << emitBoxedExpr(node.cond, analysis) << ")) {\n";
            for (const auto &c : node.cases) {
                if (c.value) {
                    out << indent << "case " << analysis.switchCaseValues.at(c.value) << ":\n";
                } else {
                    out << indent << "default:\n";
                }
                for (Stmt *inner : c.body) emitStmt(inner, out, indent + "    ", loopCounter, analysis, sourceLines, currentProcedure);
            }
            out << indent << "}\n";
        } else if constexpr (std::is_same_v<T, GotoStmt>) {
            out << indent << "goto " << mangle(node.label) << ";\n";
        } else if constexpr (std::is_same_v<T, LabelStmt>) {
            out << indent << mangle(node.name) << ": ;\n";
        } else if constexpr (std::is_same_v<T, CallStmt>) {
            const ProcedureSignature *signature = procedureSignature(node.name, analysis);
            out << indent << "(void)" << mangle(node.name) << "(";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) out << ", ";
                out << (signature && signature->nativeTyped
                        ? emitRawExpr(node.args[i], analysis)
                        : emitBoxedExpr(node.args[i], analysis));
            }
            out << ");\n";
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (currentProcedure && currentProcedure->nativeTyped) {
                if (currentProcedure->returnType.kind == TypeKind::Void) {
                    out << indent << "return;\n";
                } else {
                    out << indent << "return " << emitRawExpr(node.expr, analysis) << ";\n";
                }
            } else {
                out << indent << "return " << emitBoxedExpr(node.expr, analysis) << ";\n";
            }
        }
    }, s->node);
}

std::string emitProcedureDeclaration(const ProcedureStmt &proc,
                                     const AnalysisResult &analysis) {
    const ProcedureSignature *signature = procedureSignature(proc.name, analysis);
    bool typed = signature && signature->nativeTyped;

    std::string parameters;
    for (size_t i = 0; i < proc.params.size(); ++i) {
        if (i > 0) parameters += ", ";
        if (typed && i < signature->parameterTypes.size()) {
            parameters += emitCDeclaration(signature->parameterTypes[i], mangle(proc.params[i].name));
        } else {
            parameters += "PsValue " + mangle(proc.params[i].name);
        }
    }
    if (parameters.empty()) parameters = "void";

    if (typed) {
        return emitCDeclaration(signature->returnType,
                                mangle(proc.name) + "(" + parameters + ")");
    }
    return "PsValue " + mangle(proc.name) + "(" + parameters + ")";
}

void emitProcedure(const ProcedureStmt &proc, std::ostream &out,
                   const AnalysisResult &analysis,
                   const std::unordered_map<int, std::string> *sourceLines) {
    const ProcedureSignature *signature = procedureSignature(proc.name, analysis);
    out << emitProcedureDeclaration(proc, analysis) << " {\n";

    std::set<std::string> localVars;
    collectVars(proc.body, localVars, analysis);
    for (const auto &v : localVars) out << "    PsValue " << mangle(v) << ";\n";
    if (!localVars.empty()) out << "\n";

    int loopCounter = 0;
    for (Stmt *inner : proc.body) {
        emitStmt(inner, out, "    ", loopCounter, analysis, sourceLines, signature);
    }

    if (!signature || !signature->nativeTyped) {
        out << "    return ps_int(0L);\n";
    } else if (signature->returnType.kind != TypeKind::Void) {
        // A typed non-void body whose last statement is not a Return (for
        // example a loop that returns inside) may still end its lowered C
        // textually without a return. Sema guarantees the end is unreachable,
        // so this guard is dead, but it keeps the generated function
        // well-formed for compilers that do not prove loop guarantees.
        bool lastIsReturn = !proc.body.empty() &&
            std::holds_alternative<ReturnStmt>(proc.body.back()->node);
        if (!lastIsReturn) {
            out << "    return (" << emitCType(signature->returnType) << "){0};\n";
        }
    }
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
    out << "#include \"plainspeak_runtime.h\"\n#include <assert.h>\n#include <complex.h>\n#include <ctype.h>\n#include <stdatomic.h>\n";
    out << "typedef void *PsNullptr;\n\n";

    for (const auto &v : vars) out << "PsValue " << mangle(v) << ";\n";

    // Enumeration definitions come first so any later aggregate/object/function
    // use sees a complete native C enum type. Enumerator C names are qualified
    // by their PlainSpeak enumeration so source enums may reuse member names.
    for (Stmt *s : program) {
        auto *enumeration = std::get_if<EnumerationStmt>(&s->node);
        if (!enumeration) continue;
        auto valuesIt = analysis.enumerationValues.find(s);
        if (valuesIt == analysis.enumerationValues.end()) continue;
        out << "enum " << mangle(enumeration->name) << " {\n";
        for (std::size_t i = 0; i < valuesIt->second.size(); ++i) {
            const auto &enumerator = valuesIt->second[i];
            out << "    " << mangleEnumerator(enumeration->name, enumerator.first)
                << " = " << enumerator.second;
            if (i + 1 != valuesIt->second.size()) out << ",";
            out << "\n";
        }
        out << "};\n";
    }
    bool hasEnumerations = false;
    for (Stmt *s : program) {
        if (analysis.enumerationValues.count(s)) { hasEnumerations = true; break; }
    }
    if (hasEnumerations) out << "\n";

    // Emit complete native aggregate definitions before objects and function
    // prototypes so by-value uses have real C layout. Pointer fields may
    // mention later tags because C permits incomplete pointed-to aggregates.
    for (Stmt *s : program) {
        if (auto *structure = std::get_if<StructureStmt>(&s->node)) {
            auto fieldsIt = analysis.structureFields.find(s);
            if (fieldsIt == analysis.structureFields.end()) continue;
            out << "struct " << mangle(structure->name) << " {\n";
            for (const auto &field : fieldsIt->second) {
                out << "    " << emitAggregateFieldDeclaration(field) << ";\n";
            }
            out << "};\n";
        } else if (auto *uni = std::get_if<UnionStmt>(&s->node)) {
            auto fieldsIt = analysis.unionFields.find(s);
            if (fieldsIt == analysis.unionFields.end()) continue;
            out << "union " << mangle(uni->name) << " {\n";
            for (const auto &field : fieldsIt->second) {
                out << "    " << emitAggregateFieldDeclaration(field) << ";\n";
            }
            out << "};\n";
        }
    }
    bool hasAggregates = false;
    for (Stmt *s : program) {
        if (analysis.structureFields.count(s) || analysis.unionFields.count(s)) {
            hasAggregates = true;
            break;
        }
    }
    if (hasAggregates) out << "\n";

    // Direct top-level native declarations are real file-scope C objects so
    // procedures can take their address or access them. Their possibly-dynamic
    // PlainSpeak initializers are executed in main below.
    for (Stmt *s : program) {
        if (auto *decl = std::get_if<NativeDeclStmt>(&s->node)) {
            if (decl->alignment) out << "_Alignas(" << *decl->alignment << ") ";
            if (decl->threadLocal) out << "_Thread_local ";
            if (decl->constexprObject) out << "const ";
            out << emitCDeclaration(analysis.declarationTypes.at(s), mangle(decl->name));
            if (decl->constexprObject && decl->initializer)
                out << " = " << emitRawExpr(decl->initializer, analysis);
            out << ";\n";
        }
    }
    if (!vars.empty()) out << "\n";

    // Prototypes make forward and mutually recursive procedure calls valid C.
    for (Stmt *s : program) {
        if (auto *proc = std::get_if<ProcedureStmt>(&s->node)) {
            out << emitProcedureDeclaration(*proc, analysis) << ";\n";
        }
    }
    bool hasProcedures = false;
    for (Stmt *s : program) {
        if (std::holds_alternative<ProcedureStmt>(s->node)) { hasProcedures = true; break; }
    }
    if (hasProcedures) out << "\n";

    for (Stmt *s : program) {
        if (auto *proc = std::get_if<ProcedureStmt>(&s->node)) {
            emitProcedure(*proc, out, analysis, sourceLines);
        }
    }

    out << "int main(void) {\n";
    int loopCounter = 0;
    for (Stmt *s : program) {
        if (std::holds_alternative<ProcedureStmt>(s->node) ||
            std::holds_alternative<StructureStmt>(s->node) ||
            std::holds_alternative<UnionStmt>(s->node) ||
            std::holds_alternative<EnumerationStmt>(s->node)) continue;
        if (auto *decl = std::get_if<NativeDeclStmt>(&s->node)) {
            if (decl->initializer && !decl->constexprObject) {
                out << "    " << mangle(decl->name) << " = "
                    << emitRawExpr(decl->initializer, analysis) << ";\n";
            } else if (decl->aggregateInitializer) {
                emitAggregateStores(mangle(decl->name), analysis.declarationTypes.at(s),
                                    *decl->aggregateInitializer, out, "    ", analysis);
            }
            continue;
        }
        emitStmt(s, out, "    ", loopCounter, analysis, sourceLines);
    }
    out << "    return 0;\n";
    out << "}\n";
    return out.str();
}
