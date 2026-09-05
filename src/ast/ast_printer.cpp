#include "ast_printer.h"
#include <cstdio>
#include <sstream>

namespace {

std::string printTypeSpec(const TypeSpec &type) {
    std::string prefix;
    if (type.qualifiers.isConst) prefix += "constant ";
    if (type.qualifiers.isVolatile) prefix += "volatile ";
    if (type.qualifiers.isRestrict) prefix += "restricted ";
    if (type.qualifiers.isAtomic) prefix += "atomic ";

    std::string body;
    switch (type.kind) {
        case TypeSpecKind::Void: body = "void"; break;
        case TypeSpecKind::Boolean: body = "boolean"; break;
        case TypeSpecKind::Character: body = "character"; break;
        case TypeSpecKind::SignedCharacter: body = "signed character"; break;
        case TypeSpecKind::UnsignedCharacter: body = "unsigned character"; break;
        case TypeSpecKind::ShortInteger: body = "short integer"; break;
        case TypeSpecKind::UnsignedShortInteger: body = "unsigned short integer"; break;
        case TypeSpecKind::Integer: body = "integer"; break;
        case TypeSpecKind::UnsignedInteger: body = "unsigned integer"; break;
        case TypeSpecKind::LongInteger: body = "long integer"; break;
        case TypeSpecKind::UnsignedLongInteger: body = "unsigned long integer"; break;
        case TypeSpecKind::LongLongInteger: body = "long long integer"; break;
        case TypeSpecKind::UnsignedLongLongInteger: body = "unsigned long long integer"; break;
        case TypeSpecKind::Float: body = "float"; break;
        case TypeSpecKind::Decimal: body = "decimal"; break;
        case TypeSpecKind::LongDecimal: body = "long decimal"; break;
        case TypeSpecKind::Pointer:
            body = std::string("pointer to ") + (type.pointee ? printTypeSpec(*type.pointee) : "void");
            break;
        case TypeSpecKind::Array:
            body = std::string("array of ") + (type.pointee ? printTypeSpec(*type.pointee) : "void") +
                   " with length " + std::to_string(type.arrayBound);
            break;
        case TypeSpecKind::Structure:
            body = "structure " + type.tag; break;
        case TypeSpecKind::Union:
            body = "union " + type.tag; break;
        case TypeSpecKind::Enumeration:
            body = "enumeration " + type.tag; break;
        case TypeSpecKind::Nullptr:
            body = "null pointer type"; break;
        case TypeSpecKind::Alias:
            body = type.tag; break;
        case TypeSpecKind::TypeOf:
            body = "type of " + type.typeOfName; break;
        case TypeSpecKind::TypeOfUnqual:
            body = "type of unqualified " + type.typeOfName; break;
        case TypeSpecKind::Auto:
            body = "auto"; break;
        case TypeSpecKind::BitInt:
            body = std::string(type.bitIntUnsigned ? "unsigned " : "") + "bit integer with width " + std::to_string(type.bitWidth); break;
        case TypeSpecKind::SizeType:
            body = "size type"; break;
    }
    if (body.empty()) body = "<unknown type>";
    return prefix + body;
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
        else if constexpr (std::is_same_v<T, NullptrLit>) return "null pointer";
        else if constexpr (std::is_same_v<T, VarRef>) return node.name;
        else if constexpr (std::is_same_v<T, AddressOfExpr>) return "Address of " + node.name;
        else if constexpr (std::is_same_v<T, DerefExpr>) return "Value at " + printExpr(node.pointer);
        else if constexpr (std::is_same_v<T, ConditionalExpr>) {
            return "Choose " + printExpr(node.whenTrue) + " when " +
                   printExpr(node.condition) + " otherwise " + printExpr(node.whenFalse);
        }
        else if constexpr (std::is_same_v<T, IncDecExpr>) {
            const bool increment = node.kind == IncDecKind::PrefixIncrement ||
                                   node.kind == IncDecKind::PostfixIncrement;
            const bool prefix = node.kind == IncDecKind::PrefixIncrement ||
                                node.kind == IncDecKind::PrefixDecrement;
            return std::string(increment ? "Increment " : "Decrement ") +
                   (prefix ? "before " : "after ") + printExpr(node.operand);
        }
        else if constexpr (std::is_same_v<T, CastExpr>) return "Convert " + printExpr(node.operand) + " to type " + printTypeSpec(node.target);
        else if constexpr (std::is_same_v<T, ElementExpr>) return "Element at " + printExpr(node.index) + " in " + printExpr(node.base);
        else if constexpr (std::is_same_v<T, MemberExpr>) return "Member " + node.name + " of " + printExpr(node.base);
        else if constexpr (std::is_same_v<T, EnumeratorExpr>) return "Enumerator " + node.name + " of " + node.enumeration;
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
        } else if constexpr (std::is_same_v<T, SizeOfExpr>) {
            return "Size of " + printExpr(node.operand);
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
                          : node.op == BinOp::ShiftLeft ? "shifted left by"
                          : node.op == BinOp::ShiftRight ? "shifted right by"
                          : node.op == BinOp::Gt  ? "is greater than"
                          : node.op == BinOp::Lt  ? "is less than"
                          : node.op == BinOp::Eq  ? "is equal to"
                          : node.op == BinOp::Ne  ? "is not equal to"
                          : node.op == BinOp::Ge  ? "is greater than or equal to"
                          : node.op == BinOp::Le  ? "is less than or equal to"
                          : node.op == BinOp::BitAnd ? "bitwise and"
                          : node.op == BinOp::BitXor ? "bitwise xor"
                          : node.op == BinOp::BitOr ? "bitwise or"
                          : node.op == BinOp::And ? "and" : "or";
            return "(" + printExpr(node.lhs) + " " + std::string(op) + " " + printExpr(node.rhs) + ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            const char *op = node.op == UnaryOp::Not ? "not" :
                             node.op == UnaryOp::BitNot ? "bitwise not" : "minus";
            return std::string("(") + op + " " + printExpr(node.rhs) + ")";
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
        if constexpr (std::is_same_v<T, SayStmt>) {
            std::string out = "Say ";
            for (size_t i = 0; i < node.args.size(); ++i) {
                if (i > 0) out += " followed by ";
                out += printExpr(node.args[i]);
            }
            return out + ". ";
        } else if constexpr (std::is_same_v<T, SetStmt>) return "Set " + node.name + " to " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, StructureStmt>) {
            std::string out = "Structure " + node.name + ": ";
            for (const auto &field : node.fields) {
                if (field.bitWidth) {
                    out += "Bit field ";
                    if (!field.name.empty()) out += field.name + " ";
                    out += "as " + printTypeSpec(field.type) + " with width " + std::to_string(*field.bitWidth) + ". ";
                } else if (field.flexibleArray) {
                    out += "Flexible field " + field.name + " as " + printTypeSpec(field.type) + ". ";
                } else {
                    out += "Field " + field.name + " as " + printTypeSpec(field.type) + ". ";
                }
            }
            return out + "End structure. ";
        }
        else if constexpr (std::is_same_v<T, UnionStmt>) {
            std::string out = "Union " + node.name + ": ";
            for (const auto &field : node.fields) {
                if (field.bitWidth) {
                    out += "Bit field ";
                    if (!field.name.empty()) out += field.name + " ";
                    out += "as " + printTypeSpec(field.type) + " with width " + std::to_string(*field.bitWidth) + ". ";
                } else if (field.flexibleArray) {
                    out += "Flexible field " + field.name + " as " + printTypeSpec(field.type) + ". ";
                } else {
                    out += "Field " + field.name + " as " + printTypeSpec(field.type) + ". ";
                }
            }
            return out + "End union. ";
        }
        else if constexpr (std::is_same_v<T, EnumerationStmt>) {
            std::string out = "Enumeration " + node.name + ": ";
            for (const auto &enumerator : node.enumerators) {
                out += "Enumerator " + enumerator.name;
                if (enumerator.explicitValue) {
                    if (*enumerator.explicitValue < 0) out += " as minus " + std::to_string(-*enumerator.explicitValue);
                    else out += " as " + std::to_string(*enumerator.explicitValue);
                }
                out += ". ";
            }
            return out + "End enumeration. ";
        }
        else if constexpr (std::is_same_v<T, NativeDeclStmt>) {
            std::string out = "Declare " + node.name + " as " + printTypeSpec(node.type);
            if (node.initializer) {
                out += " with value " + printExpr(node.initializer);
            } else if (node.aggregateInitializer) {
                const auto &aggregate = *node.aggregateInitializer;
                if (aggregate.kind == AggregateInitKind::Positional) out += " with values ";
                else if (aggregate.kind == AggregateInitKind::Members) out += " with members ";
                else out += " with elements ";
                for (size_t i = 0; i < aggregate.entries.size(); ++i) {
                    if (i) out += " followed by ";
                    const auto &entry = aggregate.entries[i];
                    if (aggregate.kind == AggregateInitKind::Members) out += entry.memberName + " as ";
                    else if (aggregate.kind == AggregateInitKind::Elements) out += "at " + std::to_string(entry.elementIndex) + " as ";
                    out += printExpr(entry.expr);
                }
                out += " done";
            }
            return out + ". ";
        } else if constexpr (std::is_same_v<T, StoreThroughStmt>) {
            return "Set value at " + printExpr(node.pointer) + " to " + printExpr(node.expr) + ". ";
        } else if constexpr (std::is_same_v<T, StoreElementStmt>) {
            return "Set element at " + printExpr(node.index) + " in " + printExpr(node.base) + " to " + printExpr(node.expr) + ". ";
        } else if constexpr (std::is_same_v<T, StoreMemberStmt>) {
            return "Set member " + node.name + " of " + printExpr(node.base) + " to " + printExpr(node.expr) + ". ";
        } else if constexpr (std::is_same_v<T, AddStmt>) return "Add " + printExpr(node.expr) + " to " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, SubStmt>) return "Subtract " + printExpr(node.expr) + " from " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReadStmt>) return "Read " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReadFloatStmt>) return "ReadFloat " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, AppendStmt>) return "Append " + printExpr(node.expr) + " to " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, ReplaceItemStmt>) return "Replace item at " + printExpr(node.index) + " in " + node.varName + " with " + printExpr(node.expr) + ". ";
        else if constexpr (std::is_same_v<T, RemoveItemStmt>) return "Remove item at " + printExpr(node.index) + " from " + node.varName + ". ";
        else if constexpr (std::is_same_v<T, CommentStmt>) return "(" + node.text + ") ";
        else if constexpr (std::is_same_v<T, BreakStmt>) return "Break. ";
        else if constexpr (std::is_same_v<T, ContinueStmt>) return "Continue. ";
        else if constexpr (std::is_same_v<T, GotoStmt>) return "Go to " + node.label + ". ";
        else if constexpr (std::is_same_v<T, LabelStmt>) return "Label " + node.name + ". ";
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
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            std::string out = "Do: ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End do while " + printExpr(node.cond) + ". ";
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            std::string out = "For each " + node.itemName + " in " + printExpr(node.list) + ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End for. ";
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            std::string out = "For " + node.varName + " from " + printExpr(node.from) +
                              (node.descending ? " down to " : " to ") + printExpr(node.to) + ": ";
            for (Stmt *inner : node.body) out += printStmt(inner);
            return out + "End for. ";
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            std::string out = "Switch " + printExpr(node.cond) + ": ";
            for (const auto &c : node.cases) {
                out += (c.value ? "When " + printExpr(c.value) + ": " : "Otherwise: ");
                for (Stmt *inner : c.body) out += printStmt(inner);
            }
            return out + "End switch. ";
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
        } else if constexpr (std::is_same_v<T, ReturnStmt>) {
            return node.expr ? "Return " + printExpr(node.expr) + ". " : "Return. ";
        }
        else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::string out = "Procedure " + node.name;
            if (!node.params.empty()) {
                out += " takes ";
                for (size_t i = 0; i < node.params.size(); ++i) {
                    if (i) out += ", ";
                    out += node.params[i].name;
                    if (node.params[i].type) out += " as " + printTypeSpec(*node.params[i].type);
                }
            }
            if (node.returnType) out += " returns " + printTypeSpec(*node.returnType);
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
