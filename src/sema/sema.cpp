#include "sema.h"
#include <type_traits>
#include <utility>

namespace {

bool isNumeric(const Type &t) { return t.isNumeric(); }
bool isList(const Type &t) { return t.isList(); }
bool isArithmeticScalar(const Type &t) {
    return t.kind == TypeKind::Boolean || t.isNumeric();
}

bool isIntegralType(const Type &t) {
    return t.kind == TypeKind::Boolean || t.isInteger();
}

Type decayArray(Type t) {
    if (t.isArray() && t.elementType) return Type::pointerTo(*t.elementType);
    return t;
}

bool hasCompletePointee(const Type &t) {
    return t.isPointer() && t.elementType && t.elementType->kind != TypeKind::Void &&
           t.elementType->kind != TypeKind::Function;
}

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
            return "array of " + (t.elementType ? typeToString(*t.elementType) : std::string("unknown")) +
                   (t.arrayBound ? " with length " + std::to_string(*t.arrayBound) : std::string(" with unknown length"));
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

bool isVoidPointer(const Type &t) {
    return t.isPointer() && t.elementType && t.elementType->kind == TypeKind::Void;
}

bool pointersCompatible(const Type &target, const Type &source) {
    if (!target.isPointer() || !source.isPointer()) return false;
    if (target == source) return true;
    return isVoidPointer(target) || isVoidPointer(source);
}

bool assignableTo(const Type &target, const Type &source) {
    if (target.isArray()) return false;
    Type valueSource = decayArray(source);
    if (target == valueSource) return true;
    if (isArithmeticScalar(target) && isArithmeticScalar(valueSource)) return true;
    if (pointersCompatible(target, valueSource)) return true;
    return false;
}

bool supportsCObjectQuery(const Type &t) {
    if (t.isArray()) {
        return t.arrayBound && t.elementType && supportsCObjectQuery(*t.elementType) &&
               t.elementType->kind != TypeKind::Void && t.elementType->kind != TypeKind::Function;
    }
    return t.kind == TypeKind::Boolean || t.kind == TypeKind::Integer ||
           t.kind == TypeKind::Floating || t.kind == TypeKind::Pointer ||
           t.kind == TypeKind::Enumeration || t.kind == TypeKind::BitInt ||
           t.kind == TypeKind::Structure || t.kind == TypeKind::Union;
}

} // namespace

AnalysisResult Sema::analyze(const std::vector<Stmt *> &program) {
    AnalysisResult result;
    scopes_.clear();
    procTable_.clear();
    scopes_.emplace_back();
    analysis_ = &result;

    for (Stmt *s : program) checkStmt(s, result.diagnostics);

    scopes_.pop_back();
    analysis_ = nullptr;
    return result;
}

std::vector<Diag> Sema::check(const std::vector<Stmt *> &program) {
    return analyze(program).diagnostics;
}

void Sema::enterScope() { scopes_.emplace_back(); }
void Sema::leaveScope() { scopes_.pop_back(); }

Sema::Symbol *Sema::findVar(const std::string &name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) return &found->second;
    }
    return nullptr;
}

std::pair<Sema::Symbol, bool> Sema::lookupVar(const std::string &name, int line,
                                              std::vector<Diag> &diags) {
    if (Symbol *symbol = findVar(name)) return {*symbol, true};
    diags.push_back({1, line, "I don't know what to do with \"" + name +
                              "\" — it is used here but never declared. Use Set or Declare to create it first."});
    return {Symbol{Type::number(), false}, false};
}

bool Sema::declareVar(const std::string &name, Type type, bool nativeObject,
                      int line, std::vector<Diag> &diags) {
    auto &current = scopes_.back();
    if (current.count(name)) {
        diags.push_back({6, line, "variable \"" + name + "\" is already declared in this scope"});
        return false;
    }
    current[name] = Symbol{std::move(type), nativeObject};
    return true;
}

Type Sema::resolveTypeSpec(const TypeSpec &spec) const {
    switch (spec.kind) {
        case TypeSpecKind::Void: return Type::voidType();
        case TypeSpecKind::Boolean: return Type::boolean();
        case TypeSpecKind::Character: return Type::character();
        case TypeSpecKind::SignedCharacter: return Type::integer(IntegerRank::Char, false);
        case TypeSpecKind::UnsignedCharacter: return Type::integer(IntegerRank::Char, true);
        case TypeSpecKind::ShortInteger: return Type::integer(IntegerRank::Short, false);
        case TypeSpecKind::UnsignedShortInteger: return Type::integer(IntegerRank::Short, true);
        case TypeSpecKind::Integer: return Type::integer(IntegerRank::Int, false);
        case TypeSpecKind::UnsignedInteger: return Type::integer(IntegerRank::Int, true);
        case TypeSpecKind::LongInteger: return Type::integer(IntegerRank::Long, false);
        case TypeSpecKind::UnsignedLongInteger: return Type::integer(IntegerRank::Long, true);
        case TypeSpecKind::LongLongInteger: return Type::integer(IntegerRank::LongLong, false);
        case TypeSpecKind::UnsignedLongLongInteger: return Type::integer(IntegerRank::LongLong, true);
        case TypeSpecKind::Float: return Type::floating(FloatingRank::Float);
        case TypeSpecKind::Decimal: return Type::floating(FloatingRank::Double);
        case TypeSpecKind::LongDecimal: return Type::floating(FloatingRank::LongDouble);
        case TypeSpecKind::Pointer:
            if (spec.pointee) return Type::pointerTo(resolveTypeSpec(*spec.pointee));
            return Type::pointerTo(Type::voidType());
        case TypeSpecKind::Array:
            if (spec.pointee) return Type::arrayOf(resolveTypeSpec(*spec.pointee), spec.arrayBound);
            return Type::arrayOf(Type::voidType(), spec.arrayBound);
    }
    return Type::voidType();
}

Type Sema::inferExpr(const Expr *e, int line, std::vector<Diag> &diags) {
    Type result = std::visit([&](auto &&node) -> Type {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) return Type::number();
        else if constexpr (std::is_same_v<T, BoolLit>) return Type::number();
        else if constexpr (std::is_same_v<T, FloatLit>) return Type::decimal();
        else if constexpr (std::is_same_v<T, StringLit>) return Type::string();
        else if constexpr (std::is_same_v<T, VarRef>) {
            auto [symbol, found] = lookupVar(node.name, line, diags);
            if (found && symbol.nativeObject && analysis_) analysis_->nativeObjectRefs.insert(e);
            return symbol.type;
        }
        else if constexpr (std::is_same_v<T, AddressOfExpr>) {
            auto [symbol, found] = lookupVar(node.name, line, diags);
            if (found && !symbol.nativeObject) {
                diags.push_back({14, line, "I can only take Address of an explicitly declared native object; \"" +
                                           node.name + "\" is a boxed PlainSpeak value."});
            }
            return Type::pointerTo(symbol.type);
        }
        else if constexpr (std::is_same_v<T, DerefExpr>) {
            Type pointer = decayArray(inferExpr(node.pointer, line, diags));
            if (!pointer.isPointer() || !pointer.elementType) {
                diags.push_back({15, line, "Value at needs a pointer, not a " + typeToString(pointer) + "."});
                return Type::number();
            }
            if (pointer.elementType->kind == TypeKind::Void) {
                diags.push_back({15, line, "I can't read Value at a pointer to void because void has no object value."});
                return Type::number();
            }
            return *pointer.elementType;
        }
        else if constexpr (std::is_same_v<T, ElementExpr>) {
            Type index = inferExpr(node.index, line, diags);
            if (!isIntegralType(index)) {
                diags.push_back({17, line, "A native array index must be an integer, not a " + typeToString(index) + "."});
            }
            Type base = decayArray(inferExpr(node.base, line, diags));
            if (!base.isPointer() || !base.elementType) {
                diags.push_back({17, line, "Element at needs a native array or pointer, not a " + typeToString(base) + "."});
                return Type::number();
            }
            if (base.elementType->kind == TypeKind::Void) {
                diags.push_back({17, line, "I can't subscript a pointer to void because it has no element size."});
                return Type::number();
            }
            return *base.elementType;
        }
        else if constexpr (std::is_same_v<T, ListExpr>) {
            Type first = inferExpr(node.items.front(), line, diags);
            if (isList(first) || first.isPointer() || first.isArray()) {
                diags.push_back({9, line, "Lists can't contain other lists, native pointers, or native arrays. Use numbers, decimals, or strings as list items."});
                first = Type::number();
            }
            for (size_t i = 1; i < node.items.size(); ++i) {
                Type item = inferExpr(node.items[i], line, diags);
                if (isList(item) || item.isPointer() || item.isArray() || item != first) {
                    diags.push_back({9, line, "Every item in a list must have the same supported scalar type; this list starts with a " +
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
            Type lhsValue = decayArray(lhs);
            Type rhsValue = decayArray(rhs);
            bool lhsPointer = lhsValue.isPointer();
            bool rhsPointer = rhsValue.isPointer();
            if (lhsPointer || rhsPointer) {
                if (node.op == BinOp::Add) {
                    if (lhsPointer && hasCompletePointee(lhsValue) && isIntegralType(rhsValue)) return lhsValue;
                    if (rhsPointer && hasCompletePointee(rhsValue) && isIntegralType(lhsValue)) return rhsValue;
                    diags.push_back({16, line, "Pointer addition needs one complete object pointer and one integer offset."});
                    return Type::number();
                }
                if (node.op == BinOp::Sub) {
                    if (lhsPointer && hasCompletePointee(lhsValue) && isIntegralType(rhsValue)) return lhsValue;
                    if (lhsPointer && rhsPointer && hasCompletePointee(lhsValue) && hasCompletePointee(rhsValue) &&
                        lhsValue.elementType && rhsValue.elementType && *lhsValue.elementType == *rhsValue.elementType) {
                        return Type::number();
                    }
                    diags.push_back({16, line, "Pointer subtraction needs a complete object pointer minus an integer, or two pointers to the same element type."});
                    return Type::number();
                }
                bool equality = node.op == BinOp::Eq || node.op == BinOp::Ne;
                bool relational = node.op == BinOp::Gt || node.op == BinOp::Lt || node.op == BinOp::Ge || node.op == BinOp::Le;
                if (equality && lhsPointer && rhsPointer && pointersCompatible(lhsValue, rhsValue)) return Type::number();
                if (relational && lhsPointer && rhsPointer && hasCompletePointee(lhsValue) && hasCompletePointee(rhsValue) &&
                    lhsValue.elementType && rhsValue.elementType && *lhsValue.elementType == *rhsValue.elementType) return Type::number();
                diags.push_back({16, line, "This pointer operation needs compatible object-pointer operands."});
                return Type::number();
            }
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
            }
            if (node.op == BinOp::Sub || node.op == BinOp::Mul || node.op == BinOp::Div || node.op == BinOp::Mod) {
                if (!isNumeric(lhs) || !isNumeric(rhs)) {
                    diags.push_back({2, line, "I can't do arithmetic on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                if (lhs.isFloating() || rhs.isFloating()) return Type::decimal();
                return Type::number();
            }
            if (node.op == BinOp::And || node.op == BinOp::Or) {
                if (lhs != Type::number() || rhs != Type::number()) {
                    diags.push_back({2, line, "I can't do logical " + std::string(node.op == BinOp::And ? "and" : "or") +
                                             " on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be numbers."});
                }
                return Type::number();
            }
            if (isList(lhs) || isList(rhs) || (lhs != rhs && !(isNumeric(lhs) && isNumeric(rhs)))) {
                diags.push_back({4, line, "I can't compare a " + typeToString(lhs) + " with a " + typeToString(rhs) +
                                         ". Both sides must be comparable scalar values."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Type rhs = inferExpr(node.rhs, line, diags);
            if (node.op == UnaryOp::Neg) {
                if (!isNumeric(rhs)) diags.push_back({2, line, "I can't negate a " + typeToString(rhs) + "."});
                return rhs;
            }
            if (rhs != Type::number()) {
                diags.push_back({2, line, "I can't apply not to a " + typeToString(rhs) + ". It must be a number."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            if (!procTable_.count(node.name)) {
                diags.push_back({7, line, "I don't know what to do with \"" + node.name +
                                         "\" — it is used here but never defined. Use Procedure to create it first."});
            } else {
                const auto &expected = procTable_[node.name];
                if (node.args.size() != expected.size()) {
                    diags.push_back({8, line, "Call to \"" + node.name + "\" expects " +
                                             std::to_string(expected.size()) + " arguments but got " +
                                             std::to_string(node.args.size()) + "."});
                }
                for (Expr *arg : node.args) {
                    Type argType = inferExpr(arg, line, diags);
                    if (argType.isPointer()) {
                        diags.push_back({16, line, "Legacy Procedure parameters cannot carry native pointers yet."});
                    }
                }
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, LengthExpr>) {
            Type operand = inferExpr(node.operand, line, diags);
            if (operand != Type::string() && !isList(operand)) {
                diags.push_back({2, line, "I can't get the length of a " + typeToString(operand) + ". It must be a string or a list."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, SizeOfTypeExpr>) {
            Type queried = resolveTypeSpec(node.type);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (!supportsCObjectQuery(queried)) {
                diags.push_back({12, line, "I can't ask for the size of " + typeToString(queried) + " because it is not a complete C object type."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, SizeOfExpr>) {
            Type queried = inferExpr(node.operand, line, diags);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (!supportsCObjectQuery(queried)) {
                diags.push_back({12, line, "I can't ask for the size of a " + typeToString(queried) + " value because it does not currently have native C object layout."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, AlignOfTypeExpr>) {
            Type queried = resolveTypeSpec(node.type);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (!supportsCObjectQuery(queried)) {
                diags.push_back({12, line, "I can't ask for the alignment of " + typeToString(queried) + " because it is not a complete C object type."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, MathCallExpr>) {
            Type arg = inferExpr(node.arg, line, diags);
            if (!isNumeric(arg)) diags.push_back({2, line, "I can't apply " + node.func + " to a " + typeToString(arg) + "."});
            return Type::decimal();
        }
        else if constexpr (std::is_same_v<T, PowExpr>) {
            Type base = inferExpr(node.base, line, diags);
            Type exp = inferExpr(node.exp, line, diags);
            if (!isNumeric(base) || !isNumeric(exp)) {
                diags.push_back({2, line, "I can't raise a " + typeToString(base) + " to the power of a " + typeToString(exp) + "."});
            }
            return Type::decimal();
        }
        return Type::number();
    }, e->node);

    if (analysis_) analysis_->exprTypes[e] = result;
    return result;
}

void Sema::checkStmt(const Stmt *s, std::vector<Diag> &diags) {
    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            Type type = inferExpr(node.expr, s->line, diags);
            if (type.isPointer() || type.isArray()) {
                diags.push_back({16, s->line, "Say does not format native pointers or whole arrays; say an Element or Value instead."});
            }
        }
        else if constexpr (std::is_same_v<T, SetStmt>) {
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (Symbol *existing = findVar(node.name)) {
                bool ok = existing->nativeObject ? assignableTo(existing->type, exprType)
                                                 : existing->type == exprType;
                if (!ok) {
                    diags.push_back({3, s->line, "I can't set \"" + node.name + "\", which is a " +
                                             typeToString(existing->type) + ", to a " + typeToString(exprType) + "."});
                }
                if (existing->nativeObject && analysis_) analysis_->nativeMutationTargets.insert(s);
            } else if (exprType.isPointer() || exprType.isArray()) {
                diags.push_back({13, s->line, "Native pointers and arrays need explicit declarations; inferred Set cannot create or copy them."});
            } else {
                declareVar(node.name, exprType, false, s->line, diags);
            }
        }
        else if constexpr (std::is_same_v<T, NativeDeclStmt>) {
            Type declared = resolveTypeSpec(node.type);
            if (analysis_) analysis_->declarationTypes[s] = declared;
            if (declared.kind == TypeKind::Void) {
                diags.push_back({13, s->line, "I can't Declare \"" + node.name + "\" as void because void is not an object type."});
                return;
            }
            if (declared.isArray() && (!declared.elementType || !supportsCObjectQuery(*declared.elementType))) {
                diags.push_back({17, s->line, "A fixed native array needs a complete object element type."});
                return;
            }
            bool created = declareVar(node.name, declared, true, s->line, diags);
            if (node.initializer) {
                std::size_t diagnosticCount = diags.size();
                Type init = inferExpr(node.initializer, s->line, diags);
                bool initializerFailed = diags.size() != diagnosticCount;
                if (declared.isArray()) {
                    if (!initializerFailed) {
                        diags.push_back({17, s->line, "Whole-array initializers are not implemented yet; Declare the array and Set its elements individually."});
                    }
                } else if (created && !initializerFailed && !assignableTo(declared, init)) {
                    diags.push_back({13, s->line, "I can't initialize native object \"" + node.name +
                                              "\" of type " + typeToString(declared) + " with a " + typeToString(init) + "."});
                }
            }
        }
        else if constexpr (std::is_same_v<T, StoreThroughStmt>) {
            Type pointer = inferExpr(node.pointer, s->line, diags);
            Type value = inferExpr(node.expr, s->line, diags);
            if (!pointer.isPointer() || !pointer.elementType) {
                diags.push_back({15, s->line, "Set value at needs a pointer target, not a " + typeToString(pointer) + "."});
            } else if (pointer.elementType->kind == TypeKind::Void) {
                diags.push_back({15, s->line, "I can't store a value through a pointer to void without a concrete pointee type."});
            } else if (!assignableTo(*pointer.elementType, value)) {
                diags.push_back({13, s->line, "I can't store a " + typeToString(value) + " through a " + typeToString(pointer) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, StoreElementStmt>) {
            Type index = inferExpr(node.index, s->line, diags);
            if (!isIntegralType(index)) {
                diags.push_back({17, s->line, "A native array index must be an integer, not a " + typeToString(index) + "."});
            }
            Type base = decayArray(inferExpr(node.base, s->line, diags));
            Type value = inferExpr(node.expr, s->line, diags);
            if (!base.isPointer() || !base.elementType) {
                diags.push_back({17, s->line, "Set element at needs a native array or pointer target, not a " + typeToString(base) + "."});
            } else if (base.elementType->kind == TypeKind::Void) {
                diags.push_back({17, s->line, "I can't store an element through a pointer to void."});
            } else if (base.elementType->isArray()) {
                diags.push_back({17, s->line, "Whole-array element assignment is not implemented yet."});
            } else if (!assignableTo(*base.elementType, value)) {
                diags.push_back({13, s->line, "I can't store a " + typeToString(value) + " as an element of " + typeToString(base) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, AddStmt> || std::is_same_v<T, SubStmt>) {
            auto [symbol, found] = lookupVar(node.varName, s->line, diags);
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (found && symbol.nativeObject) {
                if (symbol.type.isPointer()) {
                    if (!hasCompletePointee(symbol.type) || !isIntegralType(exprType)) {
                        diags.push_back({16, s->line, "Changing a pointer with Add/Subtract needs a complete object pointer and an integer offset."});
                    }
                } else if (!isArithmeticScalar(symbol.type) || !isArithmeticScalar(exprType)) {
                    diags.push_back({3, s->line, "I can only change native arithmetic objects or object pointers with Add/Subtract; \"" +
                                             node.varName + "\" is a " + typeToString(symbol.type) + "."});
                }
                if (analysis_) analysis_->nativeMutationTargets.insert(s);
            } else if (found && symbol.type != exprType) {
                diags.push_back({3, s->line, std::string(std::is_same_v<T, AddStmt> ? "I can't add a " : "I can't subtract a ") +
                                         typeToString(exprType) + (std::is_same_v<T, AddStmt> ? " to \"" : " from \"") +
                                         node.varName + "\" which is a " + typeToString(symbol.type) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, ReadStmt>) {
            declareVar(node.varName, Type::number(), false, s->line, diags);
        }
        else if constexpr (std::is_same_v<T, ReadFloatStmt>) {
            declareVar(node.varName, Type::decimal(), false, s->line, diags);
        }
        else if constexpr (std::is_same_v<T, AppendStmt>) {
            auto [symbol, found] = lookupVar(node.varName, s->line, diags);
            Type itemType = inferExpr(node.expr, s->line, diags);
            if (found && !isList(symbol.type)) {
                diags.push_back({10, s->line, "I can only append to a list; \"" + node.varName + "\" is a " + typeToString(symbol.type) + "."});
            } else if (found && itemType != listElementType(symbol.type)) {
                diags.push_back({9, s->line, "I can't append a " + typeToString(itemType) + " to \"" + node.varName + "\" which is a " + typeToString(symbol.type) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, ReplaceItemStmt>) {
            Type indexType = inferExpr(node.index, s->line, diags);
            if (indexType != Type::number()) {
                diags.push_back({11, s->line, "A list position must be a whole number, not a " + typeToString(indexType) + "."});
            }
            auto [symbol, found] = lookupVar(node.varName, s->line, diags);
            Type itemType = inferExpr(node.expr, s->line, diags);
            if (found && !isList(symbol.type)) {
                diags.push_back({10, s->line, "I can only replace an item in a list; \"" + node.varName + "\" is a " + typeToString(symbol.type) + "."});
            } else if (found && itemType != listElementType(symbol.type)) {
                diags.push_back({9, s->line, "I can't put a " + typeToString(itemType) + " into \"" + node.varName + "\" which is a " + typeToString(symbol.type) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, RemoveItemStmt>) {
            Type indexType = inferExpr(node.index, s->line, diags);
            if (indexType != Type::number()) {
                diags.push_back({11, s->line, "A list position must be a whole number, not a " + typeToString(indexType) + "."});
            }
            auto [symbol, found] = lookupVar(node.varName, s->line, diags);
            if (found && !isList(symbol.type)) {
                diags.push_back({10, s->line, "I can only remove an item from a list; \"" + node.varName + "\" is a " + typeToString(symbol.type) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, RepeatStmt>) {
            Type countType = inferExpr(node.count, s->line, diags);
            if (countType != Type::number()) {
                diags.push_back({5, s->line, "Repeat needs a whole number of times, not a " + typeToString(countType) + "."});
            }
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            Type cond = inferExpr(node.cond, s->line, diags);
            if (cond.isPointer()) diags.push_back({16, s->line, "Pointer conditions are not implemented yet."});
            enterScope();
            for (Stmt *inner : node.thenBody) checkStmt(inner, diags);
            leaveScope();
            if (!node.elseBody.empty()) {
                enterScope();
                for (Stmt *inner : node.elseBody) checkStmt(inner, diags);
                leaveScope();
            }
        }
        else if constexpr (std::is_same_v<T, WhileStmt>) {
            Type condType = inferExpr(node.cond, s->line, diags);
            if (!isNumeric(condType)) {
                diags.push_back({5, s->line, "While needs a number condition, not a " + typeToString(condType) + "."});
            }
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        }
        else if constexpr (std::is_same_v<T, ForEachStmt>) {
            Type listType = inferExpr(node.list, s->line, diags);
            Type itemType = Type::number();
            if (!isList(listType)) {
                diags.push_back({10, s->line, "For each needs a list to walk through, not a " + typeToString(listType) + "."});
            } else {
                itemType = listElementType(listType);
            }
            enterScope();
            declareVar(node.itemName, itemType, false, s->line, diags);
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        }
        else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            std::vector<Type> paramTypes(node.params.size(), Type::number());
            procTable_[node.name] = paramTypes;
            enterScope();
            for (size_t i = 0; i < node.params.size(); ++i) {
                declareVar(node.params[i], paramTypes[i], false, s->line, diags);
            }
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
        }
        else if constexpr (std::is_same_v<T, CallStmt>) {
            if (!procTable_.count(node.name)) {
                diags.push_back({7, s->line, "I don't know what to do with \"" + node.name +
                                           "\" — it is used here but never defined. Use Procedure to create it first."});
            } else {
                const auto &expected = procTable_[node.name];
                if (node.args.size() != expected.size()) {
                    diags.push_back({8, s->line, "Call to \"" + node.name + "\" expects " +
                                             std::to_string(expected.size()) + " arguments but got " +
                                             std::to_string(node.args.size()) + "."});
                }
                for (Expr *arg : node.args) {
                    Type argType = inferExpr(arg, s->line, diags);
                    if (argType.isPointer() || argType.isArray()) diags.push_back({16, s->line, "Legacy Procedure parameters cannot carry native pointers or arrays yet."});
                }
            }
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            Type type = inferExpr(node.expr, s->line, diags);
            if (type.isPointer() || type.isArray()) diags.push_back({16, s->line, "Legacy Procedures cannot return native pointers or arrays yet."});
        }
        else if constexpr (std::is_same_v<T, CommentStmt>) {
        }
    }, s->node);
}
