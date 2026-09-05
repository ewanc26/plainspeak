#include "sema.h"
#include <algorithm>
#include <climits>
#include <limits>
#include <functional>
#include <type_traits>
#include <utility>

namespace {

bool isNumeric(const Type &t) { return t.kind == TypeKind::Enumeration || t.isNumeric(); }
bool isList(const Type &t) { return t.isList(); }
bool isArithmeticScalar(const Type &t) {
    return t.kind == TypeKind::Boolean || isNumeric(t);
}

bool isIntegralType(const Type &t) {
    return t.kind == TypeKind::Boolean || t.kind == TypeKind::Enumeration || t.isInteger();
}

bool hasAnyQualifiers(const TypeQualifiers &q) {
    return q.isConst || q.isVolatile || q.isRestrict || q.isAtomic;
}

TypeQualifiers semanticQualifiers(const TypeSpecQualifiers &q) {
    return TypeQualifiers{q.isConst, q.isVolatile, q.isRestrict, q.isAtomic};
}

Type stripTopQualifiers(Type t) {
    t.qualifiers = {};
    return t;
}

Type decayArray(Type t) {
    if (t.isArray() && t.elementType) return Type::pointerTo(*t.elementType);
    return stripTopQualifiers(std::move(t));
}

bool qualifierSuperset(const TypeQualifiers &target, const TypeQualifiers &source) {
    return (!source.isConst || target.isConst) &&
           (!source.isVolatile || target.isVolatile) &&
           (!source.isRestrict || target.isRestrict) &&
           (!source.isAtomic || target.isAtomic);
}

bool isObjectPointee(const Type &t) {
    return t.kind != TypeKind::Void && t.kind != TypeKind::Function;
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
    if (hasAnyQualifiers(t.qualifiers)) {
        std::string prefix;
        if (t.qualifiers.isConst) prefix += "constant ";
        if (t.qualifiers.isVolatile) prefix += "volatile ";
        if (t.qualifiers.isRestrict) prefix += "restricted ";
        if (t.qualifiers.isAtomic) prefix += "atomic ";
        return prefix + typeToString(stripTopQualifiers(t));
    }
    switch (t.kind) {
        case TypeKind::Void: return "void";
        case TypeKind::Boolean: return "boolean";
        case TypeKind::Integer: return integerName(t);
        case TypeKind::Floating:
            if (t.floatingRank == FloatingRank::Float) return "float";
            if (t.floatingRank == FloatingRank::LongDouble) return "long decimal";
            return "decimal";
        case TypeKind::Complex: return "complex decimal";
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

bool pointerBasesCompatible(const Type &a, const Type &b) {
    if (!a.isPointer() || !b.isPointer() || !a.elementType || !b.elementType) return false;
    Type aPointee = *a.elementType;
    Type bPointee = *b.elementType;
    Type aBase = stripTopQualifiers(aPointee);
    Type bBase = stripTopQualifiers(bPointee);
    if (aBase == bBase) return true;
    if (aBase.kind == TypeKind::Void && isObjectPointee(bBase)) return true;
    if (bBase.kind == TypeKind::Void && isObjectPointee(aBase)) return true;
    return false;
}

bool pointersAssignable(const Type &target, const Type &source) {
    Type valueTarget = stripTopQualifiers(target);
    Type valueSource = stripTopQualifiers(source);
    if (!pointerBasesCompatible(valueTarget, valueSource) ||
        !valueTarget.elementType || !valueSource.elementType) {
        return false;
    }
    return qualifierSuperset(valueTarget.elementType->qualifiers,
                             valueSource.elementType->qualifiers);
}

bool pointersComparable(const Type &a, const Type &b) {
    return pointerBasesCompatible(stripTopQualifiers(a), stripTopQualifiers(b));
}

bool checkedAddLong(long lhs, long rhs, long &out) {
    if ((rhs > 0 && lhs > LONG_MAX - rhs) ||
        (rhs < 0 && lhs < LONG_MIN - rhs)) {
        return false;
    }
    out = lhs + rhs;
    return true;
}

bool checkedSubLong(long lhs, long rhs, long &out) {
    if ((rhs < 0 && lhs > LONG_MAX + rhs) ||
        (rhs > 0 && lhs < LONG_MIN + rhs)) {
        return false;
    }
    out = lhs - rhs;
    return true;
}

bool checkedMulLong(long lhs, long rhs, long &out) {
    if (lhs == 0 || rhs == 0) {
        out = 0;
        return true;
    }
    if ((lhs == -1 && rhs == LONG_MIN) || (rhs == -1 && lhs == LONG_MIN)) {
        return false;
    }
    if (lhs > 0) {
        if (rhs > 0) {
            if (lhs > LONG_MAX / rhs) return false;
        } else if (rhs < LONG_MIN / lhs) {
            return false;
        }
    } else {
        if (rhs > 0) {
            if (lhs < LONG_MIN / rhs) return false;
        } else if (lhs < LONG_MAX / rhs) {
            return false;
        }
    }
    out = lhs * rhs;
    return true;
}

std::optional<long> integerConstantValue(const Expr *expr) {
    if (!expr) return std::nullopt;

    return std::visit([&](auto &&node) -> std::optional<long> {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) {
            return node.value;
        } else if constexpr (std::is_same_v<T, BoolLit>) {
            return node.value ? 1L : 0L;
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
            auto rhs = integerConstantValue(node.rhs);
            if (!rhs) return std::nullopt;
            if (node.op == UnaryOp::Not) return *rhs == 0 ? 1L : 0L;
            if (node.op == UnaryOp::BitNot) return ~*rhs;
            if (*rhs == LONG_MIN) return std::nullopt;
            return -*rhs;
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
            auto lhs = integerConstantValue(node.lhs);
            if (!lhs) return std::nullopt;

            // Preserve C short-circuit evaluation. An unevaluated right operand
            // does not need a translation-time value merely to determine this
            // expression's result.
            if (node.op == BinOp::And && *lhs == 0) return 0L;
            if (node.op == BinOp::Or && *lhs != 0) return 1L;

            auto rhs = integerConstantValue(node.rhs);
            if (!rhs) return std::nullopt;

            long result = 0;
            switch (node.op) {
                case BinOp::Add:
                    return checkedAddLong(*lhs, *rhs, result) ? std::optional<long>(result) : std::nullopt;
                case BinOp::Sub:
                    return checkedSubLong(*lhs, *rhs, result) ? std::optional<long>(result) : std::nullopt;
                case BinOp::Mul:
                    return checkedMulLong(*lhs, *rhs, result) ? std::optional<long>(result) : std::nullopt;
                case BinOp::Div:
                    if (*rhs == 0 || (*lhs == LONG_MIN && *rhs == -1)) return std::nullopt;
                    return *lhs / *rhs;
                case BinOp::Mod:
                    if (*rhs == 0 || (*lhs == LONG_MIN && *rhs == -1)) return std::nullopt;
                    return *lhs % *rhs;
                case BinOp::ShiftLeft: {
                    if (*lhs < 0 || *rhs < 0 || *rhs >= static_cast<long>(sizeof(long) * CHAR_BIT)) {
                        return std::nullopt;
                    }
                    if (*lhs > (LONG_MAX >> *rhs)) return std::nullopt;
                    return *lhs << *rhs;
                }
                case BinOp::ShiftRight:
                    if (*rhs < 0 || *rhs >= static_cast<long>(sizeof(long) * CHAR_BIT)) {
                        return std::nullopt;
                    }
                    return *lhs >> *rhs;
                case BinOp::Gt: return *lhs > *rhs ? 1L : 0L;
                case BinOp::Lt: return *lhs < *rhs ? 1L : 0L;
                case BinOp::Eq: return *lhs == *rhs ? 1L : 0L;
                case BinOp::Ne: return *lhs != *rhs ? 1L : 0L;
                case BinOp::Ge: return *lhs >= *rhs ? 1L : 0L;
                case BinOp::Le: return *lhs <= *rhs ? 1L : 0L;
                case BinOp::BitAnd: return *lhs & *rhs;
                case BinOp::BitXor: return *lhs ^ *rhs;
                case BinOp::BitOr: return *lhs | *rhs;
                case BinOp::And: return *rhs != 0 ? 1L : 0L;
                case BinOp::Or: return *rhs != 0 ? 1L : 0L;
            }
            return std::nullopt;
        } else if constexpr (std::is_same_v<T, ConditionalExpr>) {
            auto cond = integerConstantValue(node.condition);
            if (!cond) return std::nullopt;
            return integerConstantValue(*cond != 0 ? node.whenTrue : node.whenFalse);
        }
        return std::nullopt;
    }, expr->node);
}

bool isConstTruthyExpr(const Expr *expr) {
    auto value = integerConstantValue(expr);
    return value && *value != 0;
}

bool bodyHasLevelBreak(const std::vector<Stmt *> &stmts, int depth);
bool statementHasLevelBreak(const Stmt *s, int depth) {
    if (!s) return false;
    const StmtNode &node = s->node;
    if (std::holds_alternative<BreakStmt>(node)) return depth == 0;
    if (const auto *ifStmt = std::get_if<IfStmt>(&node)) {
        return bodyHasLevelBreak(ifStmt->thenBody, depth) ||
               bodyHasLevelBreak(ifStmt->elseBody, depth);
    }
    const int nested = depth + 1;
    if (const auto *repeat = std::get_if<RepeatStmt>(&node)) return bodyHasLevelBreak(repeat->body, nested);
    if (const auto *loop = std::get_if<WhileStmt>(&node)) return bodyHasLevelBreak(loop->body, nested);
    if (const auto *loop = std::get_if<DoWhileStmt>(&node)) return bodyHasLevelBreak(loop->body, nested);
    if (const auto *loop = std::get_if<ForStmt>(&node)) return bodyHasLevelBreak(loop->body, nested);
    if (const auto *loop = std::get_if<ForEachStmt>(&node)) return bodyHasLevelBreak(loop->body, nested);
    if (const auto *sw = std::get_if<SwitchStmt>(&node)) {
        for (const SwitchCase &clause : sw->cases) {
            if (bodyHasLevelBreak(clause.body, nested)) return true;
        }
    }
    return false;
}

bool bodyHasLevelBreak(const std::vector<Stmt *> &stmts, int depth) {
    for (const Stmt *s : stmts) {
        if (statementHasLevelBreak(s, depth)) return true;
    }
    return false;
}

// Forward reachability over a typed procedure body: can control flow reach the
// end of the function without passing through a Return? A Return transfers out
// of the procedure, so it has no successor edge; a Go to jumps directly to its
// Label; Break and Continue jump to the nearest enclosing loop/switch. Loops
// whose condition is a non-zero constant and whose body cannot break never fall
// through, and a Repeat/For with a known non-empty range can only leave through
// counted completion, which the structural model does not invent. The analysis
// is deliberately one-sided: it only reports the function end reachable when a
// real control-flow path exists, so it never rejects a procedure that always
// returns.
class ReturnPathChecker {
public:
    static bool endIsReachable(const std::vector<Stmt *> &body) {
        ReturnPathChecker checker;
        NodeId entry = checker.buildBlock(body, kFuncEnd);
        checker.resolveGotos();
        checker.sweepFrom(entry);
        return checker.reachedEnd_;
    }

private:
    using NodeId = int;
    static constexpr NodeId kFuncEnd = 0;

    std::vector<std::vector<NodeId>> successors_;
    std::vector<std::pair<NodeId, std::string>> pendingGotos_;
    std::unordered_map<std::string, NodeId> labelNodes_;
    std::vector<NodeId> breakStack_;
    std::vector<NodeId> continueStack_;
    bool reachedEnd_ = false;

    ReturnPathChecker() { successors_.emplace_back(); } // node 0 is the function end

    NodeId newNode() {
        successors_.emplace_back();
        return static_cast<NodeId>(successors_.size() - 1);
    }

    void addEdge(NodeId from, NodeId to) { successors_[from].push_back(to); }

    NodeId buildBlock(const std::vector<Stmt *> &stmts, NodeId continuation) {
        NodeId succ = continuation;
        for (std::size_t i = stmts.size(); i > 0; --i) succ = buildStmt(stmts[i - 1], succ);
        return succ;
    }

    NodeId buildStmt(const Stmt *s, NodeId continuation);

    NodeId buildSwitch(const SwitchStmt &node, NodeId continuation) {
        NodeId head = newNode();
        bool hasDefault = false;
        std::vector<NodeId> entries;
        NodeId fall = continuation;
        for (std::size_t i = node.cases.size(); i > 0; --i) {
            const SwitchCase &clause = node.cases[i - 1];
            if (clause.value == nullptr) hasDefault = true;
            breakStack_.push_back(fall);
            NodeId entry = buildBlock(clause.body, fall);
            breakStack_.pop_back();
            entries.push_back(entry);
            fall = entry;
        }
        // The backward loop stored case entries in reverse; add head edges in
        // source order. Fall-through keeps C semantics: each body's natural end
        // reaches the next case entry (or afterSwitch for the last one).
        for (std::size_t i = entries.size(); i > 0; --i) addEdge(head, entries[i - 1]);
        if (!hasDefault) addEdge(head, continuation);
        return head;
    }

    void resolveGotos() {
        for (const auto &[from, name] : pendingGotos_) {
            auto it = labelNodes_.find(name);
            if (it != labelNodes_.end()) addEdge(from, it->second);
        }
    }

    void sweepFrom(NodeId entry) {
        std::vector<bool> seen(successors_.size(), false);
        std::vector<NodeId> work{entry};
        seen[entry] = true;
        while (!work.empty()) {
            NodeId current = work.back();
            work.pop_back();
            for (NodeId next : successors_[current]) {
                if (seen[next]) continue;
                seen[next] = true;
                work.push_back(next);
            }
        }
        reachedEnd_ = seen[kFuncEnd];
    }
};

ReturnPathChecker::NodeId ReturnPathChecker::buildStmt(const Stmt *s, NodeId continuation) {
    return std::visit([&](auto &&node) -> NodeId {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, ReturnStmt>) {
            // Control leaves the procedure; the function end is not reached.
            return newNode();
        } else if constexpr (std::is_same_v<T, GotoStmt>) {
            NodeId jump = newNode();
            pendingGotos_.push_back({jump, node.label});
            return jump;
        } else if constexpr (std::is_same_v<T, BreakStmt>) {
            NodeId jump = newNode();
            if (!breakStack_.empty()) addEdge(jump, breakStack_.back());
            return jump;
        } else if constexpr (std::is_same_v<T, ContinueStmt>) {
            NodeId jump = newNode();
            if (!continueStack_.empty()) addEdge(jump, continueStack_.back());
            return jump;
        } else if constexpr (std::is_same_v<T, IfStmt>) {
            NodeId cond = newNode();
            addEdge(cond, buildBlock(node.thenBody, continuation));
            addEdge(cond, node.elseBody.empty() ? continuation : buildBlock(node.elseBody, continuation));
            return cond;
        } else if constexpr (std::is_same_v<T, SwitchStmt>) {
            return buildSwitch(node, continuation);
        } else if constexpr (std::is_same_v<T, RepeatStmt>) {
            NodeId head = newNode();
            breakStack_.push_back(continuation);
            NodeId bodyEntry = buildBlock(node.body, head);
            breakStack_.pop_back();
            addEdge(head, bodyEntry);
            std::optional<long> count = integerConstantValue(node.count);
            if (!(count && *count > 0)) addEdge(head, continuation);
            return head;
        } else if constexpr (std::is_same_v<T, WhileStmt>) {
            NodeId head = newNode();
            breakStack_.push_back(continuation);
            continueStack_.push_back(head);
            NodeId bodyEntry = buildBlock(node.body, head);
            breakStack_.pop_back();
            continueStack_.pop_back();
            addEdge(head, bodyEntry);
            if (!(isConstTruthyExpr(node.cond) && !bodyHasLevelBreak(node.body, 0))) {
                addEdge(head, continuation);
            }
            return head;
        } else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            NodeId head = newNode();
            NodeId cond = newNode();
            breakStack_.push_back(continuation);
            continueStack_.push_back(cond);
            NodeId bodyEntry = buildBlock(node.body, cond);
            breakStack_.pop_back();
            continueStack_.pop_back();
            addEdge(head, bodyEntry);
            addEdge(cond, head);
            if (!(isConstTruthyExpr(node.cond) && !bodyHasLevelBreak(node.body, 0))) {
                addEdge(cond, continuation);
            }
            return head;
        } else if constexpr (std::is_same_v<T, ForStmt>) {
            NodeId head = newNode();
            breakStack_.push_back(continuation);
            continueStack_.push_back(head);
            NodeId bodyEntry = buildBlock(node.body, head);
            breakStack_.pop_back();
            continueStack_.pop_back();
            addEdge(head, bodyEntry);
            std::optional<long> from = integerConstantValue(node.from);
            std::optional<long> to = integerConstantValue(node.to);
            bool knownRange = from && to;
            bool knownEmpty = knownRange && (node.descending ? *from < *to : *from > *to);
            // A known non-empty range always runs at least once, so it can only
            // leave through counted completion; unknown bounds may be empty.
            if (!knownRange || knownEmpty) addEdge(head, continuation);
            return head;
        } else if constexpr (std::is_same_v<T, ForEachStmt>) {
            NodeId head = newNode();
            breakStack_.push_back(continuation);
            continueStack_.push_back(head);
            NodeId bodyEntry = buildBlock(node.body, head);
            breakStack_.pop_back();
            continueStack_.pop_back();
            addEdge(head, bodyEntry);
            addEdge(head, continuation); // the enumerated list may be empty at runtime
            return head;
        } else if constexpr (std::is_same_v<T, LabelStmt>) {
            NodeId label = newNode();
            labelNodes_[node.name] = label;
            addEdge(label, continuation);
            return label;
        } else {
            // Declarations, operations, calls, comments and nested procedure
            // definitions complete normally.
            NodeId complete = newNode();
            addEdge(complete, continuation);
            return complete;
        }
    }, s->node);
}

bool isNullPointerConstantExpr(const Expr *expr) {
    if (!expr) return false;
    if (std::holds_alternative<NullptrLit>(expr->node)) return true;
    auto value = integerConstantValue(expr);
    return value && *value == 0;
}

bool assignableTo(const Type &target, const Type &source) {
    if (target.isArray()) return false;
    Type valueTarget = stripTopQualifiers(target);
    Type valueSource = decayArray(source);
    if (valueTarget == valueSource) return true;
    if (valueTarget.kind == TypeKind::Boolean && valueSource.kind == TypeKind::Nullptr) return true;
    if (valueTarget.isPointer() && valueSource.kind == TypeKind::Nullptr) return true;
    if (isArithmeticScalar(valueTarget) && isArithmeticScalar(valueSource)) return true;
    if (pointersAssignable(valueTarget, valueSource)) return true;
    return false;
}

bool isNullPointerLike(const Type &type, const Expr *expr) {
    return stripTopQualifiers(type).kind == TypeKind::Nullptr || isNullPointerConstantExpr(expr);
}

bool assignableExprTo(const Type &target, const Type &source, const Expr *expr) {
    Type valueTarget = stripTopQualifiers(target);
    return assignableTo(target, source) ||
           (valueTarget.isPointer() && isNullPointerLike(source, expr)) ||
           (valueTarget.kind == TypeKind::Nullptr && isNullPointerConstantExpr(expr));
}

Type memberTypeWithAggregateQualifiers(Type member, const TypeQualifiers &aggregateQualifiers) {
    if (member.isArray() && member.elementType) {
        Type element = *member.elementType;
        if (aggregateQualifiers.isConst) element.qualifiers.isConst = true;
        if (aggregateQualifiers.isVolatile) element.qualifiers.isVolatile = true;
        member.elementType = std::make_shared<Type>(std::move(element));
        return member;
    }
    if (aggregateQualifiers.isConst) member.qualifiers.isConst = true;
    if (aggregateQualifiers.isVolatile) member.qualifiers.isVolatile = true;
    return member;
}

bool supportsCObjectQuery(const Type &t) {
    if (t.isArray()) {
        return t.arrayBound && t.elementType && supportsCObjectQuery(*t.elementType) &&
               t.elementType->kind != TypeKind::Void && t.elementType->kind != TypeKind::Function;
    }
    return t.kind == TypeKind::Boolean || t.kind == TypeKind::Integer ||
           t.kind == TypeKind::Floating || t.kind == TypeKind::Pointer ||
           t.kind == TypeKind::Enumeration || t.kind == TypeKind::BitInt ||
           t.kind == TypeKind::Structure || t.kind == TypeKind::Union ||
           t.kind == TypeKind::Nullptr || t.kind == TypeKind::Complex;
}

Type signedInteger(IntegerRank rank) { return Type::integer(rank, false); }
Type unsignedInteger(IntegerRank rank) { return Type::integer(rank, true); }

int integerRankOrder(IntegerRank rank) {
    switch (rank) {
        case IntegerRank::Char: return 0;
        case IntegerRank::Short: return 1;
        case IntegerRank::Int: return 2;
        case IntegerRank::Long: return 3;
        case IntegerRank::LongLong: return 4;
    }
    return 0;
}

unsigned long long unsignedMaxForRank(IntegerRank rank) {
    switch (rank) {
        case IntegerRank::Char: return std::numeric_limits<unsigned char>::max();
        case IntegerRank::Short: return std::numeric_limits<unsigned short>::max();
        case IntegerRank::Int: return std::numeric_limits<unsigned int>::max();
        case IntegerRank::Long: return std::numeric_limits<unsigned long>::max();
        case IntegerRank::LongLong: return std::numeric_limits<unsigned long long>::max();
    }
    return 0;
}

unsigned long long signedMaxForRank(IntegerRank rank) {
    switch (rank) {
        case IntegerRank::Char: return static_cast<unsigned long long>(std::numeric_limits<signed char>::max());
        case IntegerRank::Short: return static_cast<unsigned long long>(std::numeric_limits<short>::max());
        case IntegerRank::Int: return static_cast<unsigned long long>(std::numeric_limits<int>::max());
        case IntegerRank::Long: return static_cast<unsigned long long>(std::numeric_limits<long>::max());
        case IntegerRank::LongLong: return static_cast<unsigned long long>(std::numeric_limits<long long>::max());
    }
    return 0;
}

Type integerPromotion(Type type) {
    type = stripTopQualifiers(std::move(type));
    if (type.kind == TypeKind::Boolean || type.kind == TypeKind::Enumeration) {
        return signedInteger(IntegerRank::Int);
    }
    if (type.kind != TypeKind::Integer) return type;

    if (integerRankOrder(type.integerRank) >= integerRankOrder(IntegerRank::Int)) {
        return type;
    }

    if (type.integerRank == IntegerRank::Char && type.charSignedness == CharSignedness::Plain) {
        if (std::numeric_limits<char>::lowest() >= std::numeric_limits<int>::lowest() &&
            std::numeric_limits<char>::max() <= std::numeric_limits<int>::max()) {
            return signedInteger(IntegerRank::Int);
        }
        return unsignedInteger(IntegerRank::Int);
    }

    const bool unsignedSource = type.isUnsigned || type.charSignedness == CharSignedness::Unsigned;
    if (!unsignedSource || unsignedMaxForRank(type.integerRank) <= signedMaxForRank(IntegerRank::Int)) {
        return signedInteger(IntegerRank::Int);
    }
    return unsignedInteger(IntegerRank::Int);
}

Type usualIntegerConversion(Type lhs, Type rhs) {
    lhs = integerPromotion(std::move(lhs));
    rhs = integerPromotion(std::move(rhs));
    if (lhs == rhs) return lhs;

    if (lhs.kind != TypeKind::Integer || rhs.kind != TypeKind::Integer) {
        return Type::number();
    }

    const int lhsRank = integerRankOrder(lhs.integerRank);
    const int rhsRank = integerRankOrder(rhs.integerRank);
    if (lhs.isUnsigned == rhs.isUnsigned) return lhsRank >= rhsRank ? lhs : rhs;

    Type unsignedType = lhs.isUnsigned ? lhs : rhs;
    Type signedType = lhs.isUnsigned ? rhs : lhs;
    const int unsignedRank = integerRankOrder(unsignedType.integerRank);
    const int signedRank = integerRankOrder(signedType.integerRank);

    if (unsignedRank >= signedRank) return unsignedType;
    if (signedMaxForRank(signedType.integerRank) >= unsignedMaxForRank(unsignedType.integerRank)) {
        return signedType;
    }
    return unsignedInteger(signedType.integerRank);
}

Type usualArithmeticConversion(Type lhs, Type rhs) {
    lhs = decayArray(std::move(lhs));
    rhs = decayArray(std::move(rhs));

    if (lhs.kind == TypeKind::Complex || rhs.kind == TypeKind::Complex)
        return Type::complex();

    if (lhs.kind == TypeKind::Floating || rhs.kind == TypeKind::Floating) {
        FloatingRank rank = FloatingRank::Float;
        auto raise = [&](const Type &t) {
            if (t.kind != TypeKind::Floating) return;
            if (t.floatingRank == FloatingRank::LongDouble) rank = FloatingRank::LongDouble;
            else if (t.floatingRank == FloatingRank::Double && rank != FloatingRank::LongDouble)
                rank = FloatingRank::Double;
        };
        raise(lhs);
        raise(rhs);
        return Type::floating(rank);
    }
    return usualIntegerConversion(std::move(lhs), std::move(rhs));
}

bool isBitwiseIntegral(const Type &type) {
    Type value = stripTopQualifiers(type);
    return value.kind == TypeKind::Boolean || value.kind == TypeKind::Enumeration ||
           value.kind == TypeKind::Integer;
}
bool isCastInteger(const Type &type) {
    Type value = stripTopQualifiers(type);
    return value.kind == TypeKind::Boolean || value.kind == TypeKind::Enumeration ||
           value.kind == TypeKind::Integer || value.kind == TypeKind::BitInt;
}

bool isCastArithmetic(const Type &type) {
    Type value = stripTopQualifiers(type);
    return isCastInteger(value) || value.kind == TypeKind::Floating;
}

bool isCastScalar(const Type &type) {
    Type value = decayArray(type);
    return isCastArithmetic(value) || value.kind == TypeKind::Pointer ||
           value.kind == TypeKind::Nullptr;
}

bool canExplicitCast(const Type &target, const Type &source, const Expr *sourceExpr) {
    Type to = stripTopQualifiers(target);
    Type from = decayArray(source);

    if (!isCastScalar(to) || !isCastScalar(from)) return false;
    if (to == from) return true;
    if (to.kind == TypeKind::Nullptr) return isNullPointerConstantExpr(sourceExpr);
    if (to.kind == TypeKind::Boolean) return true;
    if (isCastArithmetic(to) && isCastArithmetic(from)) return true;
    if (to.kind == TypeKind::Pointer && (from.kind == TypeKind::Pointer || from.kind == TypeKind::Nullptr)) return true;
    if (to.kind == TypeKind::Pointer && isCastInteger(from)) return true;
    if (isCastInteger(to) && from.kind == TypeKind::Pointer) return true;
    return false;
}

TypeQualifiers combinedQualifiers(const TypeQualifiers &a, const TypeQualifiers &b) {
    return TypeQualifiers{
        a.isConst || b.isConst,
        a.isVolatile || b.isVolatile,
        a.isRestrict || b.isRestrict,
        a.isAtomic || b.isAtomic
    };
}

std::optional<Type> conditionalPointerType(Type lhs, Type rhs) {
    lhs = decayArray(std::move(lhs));
    rhs = decayArray(std::move(rhs));
    if (!lhs.isPointer() || !rhs.isPointer() || !lhs.elementType || !rhs.elementType ||
        !pointersComparable(lhs, rhs)) {
        return std::nullopt;
    }

    Type leftPointee = *lhs.elementType;
    Type rightPointee = *rhs.elementType;
    Type leftBase = stripTopQualifiers(leftPointee);
    Type rightBase = stripTopQualifiers(rightPointee);
    Type resultPointee;

    if (leftBase.kind == TypeKind::Void && isObjectPointee(rightBase)) {
        resultPointee = leftBase;
    } else if (rightBase.kind == TypeKind::Void && isObjectPointee(leftBase)) {
        resultPointee = rightBase;
    } else if (leftBase == rightBase) {
        resultPointee = leftBase;
    } else {
        return std::nullopt;
    }

    resultPointee.qualifiers = combinedQualifiers(leftPointee.qualifiers, rightPointee.qualifiers);
    return Type::pointerTo(std::move(resultPointee));
}

bool isConditionalScalar(Type type) {
    type = decayArray(std::move(type));
    return isArithmeticScalar(type) || type.isPointer() || type.kind == TypeKind::Nullptr;
}


std::optional<std::size_t> bitFieldWidthLimit(const Type &type) {
    if (type.kind == TypeKind::Boolean) return 1;
    if (type.kind == TypeKind::Enumeration) return sizeof(int) * CHAR_BIT;
    if (type.kind != TypeKind::Integer) return std::nullopt;

    switch (type.integerRank) {
        case IntegerRank::Char: return sizeof(signed char) * CHAR_BIT;
        case IntegerRank::Short: return sizeof(short) * CHAR_BIT;
        case IntegerRank::Int: return sizeof(int) * CHAR_BIT;
        case IntegerRank::Long: return sizeof(long) * CHAR_BIT;
        case IntegerRank::LongLong: return sizeof(long long) * CHAR_BIT;
    }
    return std::nullopt;
}

} // namespace

AnalysisResult Sema::analyze(const std::vector<Stmt *> &program) {
    AnalysisResult result;
    scopes_.clear();
    procTable_.clear();
    structureTable_.clear();
    unionTable_.clear();
    enumerationTable_.clear();
    aliases_.clear();
    currentProcedure_.reset();
    loopDepth_ = 0;
    breakableDepth_ = 0;
    scopes_.emplace_back();
    analysis_ = &result;

    for (Stmt *s : program) {
        if (auto *alias = std::get_if<TypeAliasStmt>(&s->node)) {
            if (aliases_.count(alias->name))
                result.diagnostics.push_back({22, s->line, "Type alias \"" + alias->name + "\" is already defined."});
            else aliases_[alias->name] = alias->target;
        }
    }

    // Validate alias chains before declarations resolve them.
    std::function<bool(const std::string &, std::unordered_set<std::string> &, std::unordered_set<std::string> &)> validateAlias;
    validateAlias = [&](const std::string &name, auto &active, auto &checked) {
        if (checked.count(name)) return true;
        if (!active.insert(name).second) return false;
        auto it = aliases_.find(name);
        if (it == aliases_.end()) return false;
        bool valid = it->second.kind != TypeSpecKind::Alias || validateAlias(it->second.tag, active, checked);
        active.erase(name);
        if (valid) checked.insert(name);
        return valid;
    };
    std::unordered_set<std::string> checkedAliases;
    std::vector<std::string> aliasNames;
    for (const auto &[name, target] : aliases_) aliasNames.push_back(name);
    std::sort(aliasNames.begin(), aliasNames.end());
    for (const auto &name : aliasNames) {
        const auto &target = aliases_.at(name);
        std::unordered_set<std::string> active;
        if (target.kind == TypeSpecKind::Alias && !validateAlias(name, active, checkedAliases))
            result.diagnostics.push_back({22, 1, "Type alias \"" + name + "\" refers to an unknown or recursive alias."});
    }

    // Structure tags live in their own namespace. Register all tags as
    // incomplete first so self-referential and forward pointers can resolve;
    // field checking later completes them in source order.
    for (Stmt *s : program) {
        auto *structure = std::get_if<StructureStmt>(&s->node);
        if (!structure) continue;
        if (structureTable_.count(structure->name)) {
            result.diagnostics.push_back({19, s->line, "Structure \"" + structure->name + "\" is already defined."});
            continue;
        }
        structureTable_[structure->name] = StructureInfo{};
        result.structures[structure->name] = StructureInfo{};
    }

    // Union tags are a separate namespace from structure tags and follow the
    // same incomplete-first rule for self/forward pointers.
    for (Stmt *s : program) {
        auto *uni = std::get_if<UnionStmt>(&s->node);
        if (!uni) continue;
        if (unionTable_.count(uni->name)) {
            result.diagnostics.push_back({20, s->line, "Union \"" + uni->name + "\" is already defined."});
            continue;
        }
        if (structureTable_.count(uni->name)) {
            result.diagnostics.push_back({20, s->line, "Tag \"" + uni->name +
                                              "\" is already used by a Structure; C structure and union tags share one namespace."});
            continue;
        }
        unionTable_[uni->name] = StructureInfo{};
        result.unions[uni->name] = StructureInfo{};
    }

    // Enumeration tags share C's tag namespace with structures and unions.
    // Register them incomplete first so pointers can name the tag before the
    // definition, while by-value uses still require source-order completeness.
    for (Stmt *s : program) {
        auto *enumeration = std::get_if<EnumerationStmt>(&s->node);
        if (!enumeration) continue;
        if (enumerationTable_.count(enumeration->name)) {
            result.diagnostics.push_back({22, s->line, "Enumeration \"" + enumeration->name + "\" is already defined."});
            continue;
        }
        if (structureTable_.count(enumeration->name) || unionTable_.count(enumeration->name)) {
            result.diagnostics.push_back({22, s->line, "Tag \"" + enumeration->name +
                                              "\" is already used by a structure or union; C aggregate and enumeration tags share one namespace."});
            continue;
        }
        enumerationTable_[enumeration->name] = EnumerationInfo{};
        result.enumerations[enumeration->name] = EnumerationInfo{};
    }

    // Register every procedure signature before checking any body. This makes
    // source order irrelevant for calls and gives codegen enough information
    // to emit real C prototypes before definitions.
    for (Stmt *s : program) {
        auto *proc = std::get_if<ProcedureStmt>(&s->node);
        if (!proc) continue;
        if (procTable_.count(proc->name)) {
            result.diagnostics.push_back({18, s->line, "Procedure \"" + proc->name + "\" is already defined."});
            continue;
        }

        bool typed = proc->returnType.has_value();
        ProcedureSignature signature;
        signature.nativeTyped = typed;
        signature.returnType = typed ? resolveTypeSpec(*proc->returnType) : Type::number();
        if (typed) validateTypeQualifiers(signature.returnType, s->line, result.diagnostics);

        for (const auto &param : proc->params) {
            Type paramType = Type::number();
            if (typed && param.type) {
                paramType = resolveTypeSpec(*param.type);
                validateTypeQualifiers(paramType, s->line, result.diagnostics);
                if (paramType.kind == TypeKind::Void) {
                    result.diagnostics.push_back({18, s->line, "Typed parameter \"" + param.name + "\" cannot have type void."});
                    paramType = Type::number();
                } else if (paramType.isArray()) {
                    paramType = decayArray(paramType);
                }
            }
            signature.parameterTypes.push_back(std::move(paramType));
        }

        if (typed && signature.returnType.isArray()) {
            result.diagnostics.push_back({18, s->line, "A typed Procedure cannot return an array directly; return a pointer to the array instead."});
        }

        procTable_[proc->name] = signature;
        result.procedureSignatures[proc->name] = signature;
    }

    for (Stmt *s : program) checkStmt(s, result.diagnostics);

    validateLabels(result.diagnostics);

    scopes_.pop_back();
    currentProcedure_.reset();
    analysis_ = nullptr;
    return result;
}

std::vector<Diag> Sema::check(const std::vector<Stmt *> &program) {
    return analyze(program).diagnostics;
}

void Sema::enterScope() { scopes_.emplace_back(); }
void Sema::leaveScope() { scopes_.pop_back(); }

void Sema::validateLabels(std::vector<Diag> &diags) {
    for (const auto &[target, line] : gotoTargets_) {
        if (!labels_.count(target)) {
            diags.push_back({32, line, "Go to jumps to \"" + target +
                                      "\" but no Label \"" + target + "\" exists in this function."});
        }
    }
    labels_.clear();
    gotoTargets_.clear();
}

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
    if (spec.kind == TypeSpecKind::Alias) {
        auto it = aliases_.find(spec.tag);
        if (it == aliases_.end()) return Type::voidType();
        Type result = resolveTypeSpec(it->second);
        TypeQualifiers q = semanticQualifiers(spec.qualifiers);
        result.qualifiers.isConst |= q.isConst;
        result.qualifiers.isVolatile |= q.isVolatile;
        result.qualifiers.isRestrict |= q.isRestrict;
        result.qualifiers.isAtomic |= q.isAtomic;
        return result;
    }
    Type result;
    switch (spec.kind) {
        case TypeSpecKind::Void: result = Type::voidType(); break;
        case TypeSpecKind::Boolean: result = Type::boolean(); break;
        case TypeSpecKind::Character: result = Type::character(); break;
        case TypeSpecKind::SignedCharacter: result = Type::integer(IntegerRank::Char, false); break;
        case TypeSpecKind::UnsignedCharacter: result = Type::integer(IntegerRank::Char, true); break;
        case TypeSpecKind::ShortInteger: result = Type::integer(IntegerRank::Short, false); break;
        case TypeSpecKind::UnsignedShortInteger: result = Type::integer(IntegerRank::Short, true); break;
        case TypeSpecKind::Integer: result = Type::integer(IntegerRank::Int, false); break;
        case TypeSpecKind::UnsignedInteger: result = Type::integer(IntegerRank::Int, true); break;
        case TypeSpecKind::LongInteger: result = Type::integer(IntegerRank::Long, false); break;
        case TypeSpecKind::UnsignedLongInteger: result = Type::integer(IntegerRank::Long, true); break;
        case TypeSpecKind::LongLongInteger: result = Type::integer(IntegerRank::LongLong, false); break;
        case TypeSpecKind::UnsignedLongLongInteger: result = Type::integer(IntegerRank::LongLong, true); break;
        case TypeSpecKind::Float: result = Type::floating(FloatingRank::Float); break;
        case TypeSpecKind::Decimal: result = Type::floating(FloatingRank::Double); break;
        case TypeSpecKind::LongDecimal: result = Type::floating(FloatingRank::LongDouble); break;
        case TypeSpecKind::Pointer:
            result = Type::pointerTo(spec.pointee ? resolveTypeSpec(*spec.pointee) : Type::voidType());
            break;
        case TypeSpecKind::Array:
            result = Type::arrayOf(spec.pointee ? resolveTypeSpec(*spec.pointee) : Type::voidType(),
                                   spec.arrayBound);
            break;
        case TypeSpecKind::Structure: result = Type::structure(spec.tag); break;
        case TypeSpecKind::Union: result = Type::unionType(spec.tag); break;
        case TypeSpecKind::Enumeration: result = Type::enumeration(spec.tag); break;
        case TypeSpecKind::Nullptr: result = Type::nullptrType(); break;
        case TypeSpecKind::Alias: result = Type::voidType(); break;
        case TypeSpecKind::TypeOf:
        case TypeSpecKind::TypeOfUnqual: {
            result = Type::voidType();
            for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
                auto found = it->find(spec.typeOfName);
                if (found != it->end()) { result = found->second.type; break; }
            }
            if (spec.kind == TypeSpecKind::TypeOfUnqual) result.qualifiers = {};
            break;
        }
        case TypeSpecKind::Auto: result = Type::voidType(); break;
        case TypeSpecKind::BitInt: result = Type::bitInt(spec.bitWidth, spec.bitIntUnsigned); break;
        case TypeSpecKind::SizeType: result = Type::integer(IntegerRank::Long, true); break;
        case TypeSpecKind::PtrdiffType: result = Type::integer(IntegerRank::Long, false); break;
        case TypeSpecKind::Complex: result = Type::complex(); break;
    }

    TypeQualifiers q = semanticQualifiers(spec.qualifiers);
    if (result.isArray() && result.elementType) {
        // C's source-level const/volatile array declarations qualify the
        // element type. Keep illegal restrict/_Atomic qualification visible
        // on the array itself so validation can diagnose it.
        Type element = *result.elementType;
        if (q.isConst) element.qualifiers.isConst = true;
        if (q.isVolatile) element.qualifiers.isVolatile = true;
        result.elementType = std::make_shared<Type>(std::move(element));
        result.qualifiers.isRestrict = q.isRestrict;
        result.qualifiers.isAtomic = q.isAtomic;
    } else {
        result.qualifiers = q;
    }
    return result;
}

bool Sema::validateTypeQualifiers(const Type &type, int line, std::vector<Diag> &diags) const {
    bool valid = true;

    if (type.qualifiers.isRestrict) {
        if (!type.isPointer() || !type.elementType ||
            !isObjectPointee(stripTopQualifiers(*type.elementType))) {
            diags.push_back({24, line,
                "restricted may qualify only a pointer to an object type; " +
                typeToString(type) + " does not satisfy that C constraint."});
            valid = false;
        }
    }

    if (type.qualifiers.isAtomic &&
        (type.kind == TypeKind::Void || type.kind == TypeKind::Array ||
         type.kind == TypeKind::Function)) {
        diags.push_back({24, line,
            "atomic cannot qualify " + typeToString(stripTopQualifiers(type)) +
            " because C does not permit _Atomic on void, array, or function types in this source surface."});
        valid = false;
    }

    if (type.isPointer() && type.elementType) {
        valid = validateTypeQualifiers(*type.elementType, line, diags) && valid;
    } else if (type.isArray() && type.elementType) {
        valid = validateTypeQualifiers(*type.elementType, line, diags) && valid;
    } else if (type.isFunction()) {
        if (hasAnyQualifiers(type.qualifiers)) {
            diags.push_back({24, line, "A function type cannot carry PlainSpeak type qualifiers."});
            valid = false;
        }
        if (type.returnType) valid = validateTypeQualifiers(*type.returnType, line, diags) && valid;
        for (const Type &param : type.parameterTypes) {
            valid = validateTypeQualifiers(param, line, diags) && valid;
        }
    }
    return valid;
}

bool Sema::isCompleteObjectType(const Type &type) const {
    if (type.kind == TypeKind::Void || type.kind == TypeKind::Function) return false;
    if (type.kind == TypeKind::Complex) return true;
    if (type.isArray()) {
        return type.arrayBound && type.elementType &&
               isCompleteObjectType(*type.elementType) &&
               !containsFlexibleArray(*type.elementType);
    }
    if (type.kind == TypeKind::Structure) {
        auto it = structureTable_.find(type.tag);
        return it != structureTable_.end() && it->second.complete;
    }
    if (type.kind == TypeKind::Union) {
        auto it = unionTable_.find(type.tag);
        return it != unionTable_.end() && it->second.complete;
    }
    if (type.kind == TypeKind::Enumeration) {
        auto it = enumerationTable_.find(type.tag);
        return it != enumerationTable_.end() && it->second.complete;
    }
    return type.kind == TypeKind::Boolean || type.kind == TypeKind::Integer ||
           type.kind == TypeKind::Floating || type.kind == TypeKind::Pointer ||
           type.kind == TypeKind::BitInt || type.kind == TypeKind::Nullptr;
}

bool Sema::hasConstSubobject(const Type &type) const {
    if (type.qualifiers.isConst) return true;
    if (type.isArray() && type.elementType) return hasConstSubobject(*type.elementType);

    const StructureInfo *info = nullptr;
    if (type.kind == TypeKind::Structure) {
        auto it = structureTable_.find(type.tag);
        if (it != structureTable_.end()) info = &it->second;
    } else if (type.kind == TypeKind::Union) {
        auto it = unionTable_.find(type.tag);
        if (it != unionTable_.end()) info = &it->second;
    }
    if (!info || !info->complete) return false;
    for (const auto &field : info->fields) {
        if (hasConstSubobject(field.type)) return true;
    }
    return false;
}

bool Sema::isModifiableObjectType(const Type &type) const {
    if (type.isArray() || type.qualifiers.isConst) return false;
    if (type.kind == TypeKind::Structure || type.kind == TypeKind::Union) {
        return !hasConstSubobject(type);
    }
    return type.kind != TypeKind::Void && type.kind != TypeKind::Function;
}

bool Sema::containsFlexibleArray(const Type &type) const {
    if (type.isArray() && type.elementType) return containsFlexibleArray(*type.elementType);

    const StructureInfo *info = nullptr;
    if (type.kind == TypeKind::Structure) {
        auto it = structureTable_.find(type.tag);
        if (it != structureTable_.end()) info = &it->second;
    } else if (type.kind == TypeKind::Union) {
        auto it = unionTable_.find(type.tag);
        if (it != unionTable_.end()) info = &it->second;
    }

    return info && info->complete && info->hasFlexibleArray;
}

const AggregateFieldInfo *Sema::findAggregateField(const Type &base, const std::string &name) const {
    Type aggregate = base;
    if (aggregate.isPointer() && aggregate.elementType) aggregate = *aggregate.elementType;

    const StructureInfo *info = nullptr;
    if (aggregate.kind == TypeKind::Structure) {
        auto it = structureTable_.find(aggregate.tag);
        if (it != structureTable_.end()) info = &it->second;
    } else if (aggregate.kind == TypeKind::Union) {
        auto it = unionTable_.find(aggregate.tag);
        if (it != unionTable_.end()) info = &it->second;
    }
    if (!info || !info->complete) return nullptr;
    for (const auto &field : info->fields) {
        if (!field.name.empty() && field.name == name) return &field;
    }
    return nullptr;
}

bool Sema::isNativeLvalueExpr(const Expr *e) const {
    return std::visit([&](auto &&node) -> bool {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, VarRef>) {
            return analysis_ && analysis_->nativeObjectRefs.count(e) != 0;
        } else if constexpr (std::is_same_v<T, DerefExpr> ||
                             std::is_same_v<T, ElementExpr>) {
            return true;
        } else if constexpr (std::is_same_v<T, MemberExpr>) {
            if (!analysis_) return false;
            auto found = analysis_->exprTypes.find(node.base);
            if (found != analysis_->exprTypes.end() && found->second.isPointer()) return true;
            return isNativeLvalueExpr(node.base);
        }
        return false;
    }, e->node);
}

Type Sema::inferExpr(const Expr *e, int line, std::vector<Diag> &diags) {
    Type result = std::visit([&](auto &&node) -> Type {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, IntLit>) return Type::number();
        else if constexpr (std::is_same_v<T, BoolLit>) return Type::number();
        else if constexpr (std::is_same_v<T, FloatLit>) return Type::decimal();
        else if constexpr (std::is_same_v<T, StringLit>) return Type::string();
        else if constexpr (std::is_same_v<T, NullptrLit>) return Type::nullptrType();
        else if constexpr (std::is_same_v<T, VarRef>) {
            auto [symbol, found] = lookupVar(node.name, line, diags);
            if (found && symbol.nativeObject && analysis_) analysis_->nativeObjectRefs.insert(e);
            return symbol.type;
        }
        else if constexpr (std::is_same_v<T, EnumeratorExpr>) {
            auto found = enumerationTable_.find(node.enumeration);
            if (found == enumerationTable_.end()) {
                diags.push_back({22, line, "I don't know enumeration \"" + node.enumeration +
                                           "\" for Enumerator \"" + node.name + "\"."});
                return Type::integer(IntegerRank::Int);
            }
            if (!found->second.complete) {
                diags.push_back({22, line, "Enumeration \"" + node.enumeration +
                                           "\" is incomplete here, so its enumerators are not available."});
                return Type::integer(IntegerRank::Int);
            }
            for (const auto &enumerator : found->second.enumerators) {
                if (enumerator.first == node.name) return Type::integer(IntegerRank::Int);
            }
            diags.push_back({22, line, "Enumeration \"" + node.enumeration +
                                       "\" has no Enumerator \"" + node.name + "\"."});
            return Type::integer(IntegerRank::Int);
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
        else if constexpr (std::is_same_v<T, CastExpr>) {
            const std::size_t before = diags.size();
            Type source = inferExpr(node.operand, line, diags);
            Type target = resolveTypeSpec(node.target);

            if (!validateTypeQualifiers(target, line, diags)) return Type::number();
            if (diags.size() != before) return Type::number();

            if (target.kind == TypeKind::Void) {
                diags.push_back({26, line, "Convert to void is not available as a value expression yet; PlainSpeak needs a discard-expression statement surface first."});
                return Type::number();
            }
            if (target.kind == TypeKind::Enumeration && !isCompleteObjectType(target)) {
                diags.push_back({26, line, "Convert needs a complete enumeration target; " +
                                          typeToString(target) + " is incomplete here."});
                return Type::number();
            }
            if (!canExplicitCast(target, source, node.operand)) {
                diags.push_back({26, line, "I can't explicitly convert " + typeToString(source) +
                                          " to " + typeToString(target) +
                                          "; C casts here require compatible scalar conversion categories."});
                return Type::number();
            }
            return target;
        }
        else if constexpr (std::is_same_v<T, ConditionalExpr>) {
            const std::size_t before = diags.size();
            Type condition = inferExpr(node.condition, line, diags);
            Type whenTrue = inferExpr(node.whenTrue, line, diags);
            Type whenFalse = inferExpr(node.whenFalse, line, diags);
            if (diags.size() != before) return Type::number();

            if (!isConditionalScalar(condition)) {
                diags.push_back({28, line,
                    "Choose condition must have C scalar type, not " + typeToString(condition) + "."});
                return Type::number();
            }

            Type trueValue = decayArray(whenTrue);
            Type falseValue = decayArray(whenFalse);
            if (isArithmeticScalar(trueValue) && isArithmeticScalar(falseValue)) {
                return usualArithmeticConversion(trueValue, falseValue);
            }

            if (trueValue.isPointer() && isNullPointerLike(falseValue, node.whenFalse)) return trueValue;
            if (falseValue.isPointer() && isNullPointerLike(trueValue, node.whenTrue)) return falseValue;
            if (trueValue.kind == TypeKind::Nullptr && falseValue.kind == TypeKind::Nullptr) {
                return Type::nullptrType();
            }
            if (trueValue.isPointer() && falseValue.isPointer()) {
                if (auto pointer = conditionalPointerType(trueValue, falseValue)) return *pointer;
                diags.push_back({28, line,
                    "Choose pointer branches need compatible pointed-to types, not " +
                    typeToString(whenTrue) + " and " + typeToString(whenFalse) + "."});
                return Type::number();
            }

            if ((trueValue.kind == TypeKind::Structure || trueValue.kind == TypeKind::Union) &&
                trueValue == falseValue) {
                return trueValue;
            }

            diags.push_back({28, line,
                "Choose branches do not have a supported common C type: " +
                typeToString(whenTrue) + " and " + typeToString(whenFalse) + "."});
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, IncDecExpr>) {
            const std::size_t before = diags.size();
            Type operand = inferExpr(node.operand, line, diags);
            if (diags.size() != before) return Type::number();

            if (!isNativeLvalueExpr(node.operand)) {
                diags.push_back({27, line,
                    "Increment/decrement needs a native modifiable lvalue, not a temporary or boxed value."});
                return Type::number();
            }
            if (!isModifiableObjectType(operand)) {
                diags.push_back({27, line,
                    "Increment/decrement cannot modify " + typeToString(operand) +
                    " because it is not a modifiable C object type."});
                return Type::number();
            }

            Type value = stripTopQualifiers(operand);
            if (value.isPointer()) {
                if (!value.elementType || !isCompleteObjectType(*value.elementType)) {
                    diags.push_back({27, line,
                        "Increment/decrement on a pointer requires a pointer to a complete object type, not " +
                        typeToString(operand) + "."});
                    return Type::number();
                }
                return value;
            }
            if (!isArithmeticScalar(value)) {
                diags.push_back({27, line,
                    "Increment/decrement needs a real arithmetic value or complete object pointer, not " +
                    typeToString(operand) + "."});
                return Type::number();
            }
            return value;
        }
        else if constexpr (std::is_same_v<T, MemberExpr>) {
            Type base = inferExpr(node.base, line, diags);
            Type aggregate = base;
            if (aggregate.isPointer() && aggregate.elementType) aggregate = *aggregate.elementType;
            if (aggregate.kind != TypeKind::Structure && aggregate.kind != TypeKind::Union) {
                diags.push_back({19, line, "Member " + node.name + " needs a structure/union object or pointer, not a " +
                                           typeToString(base) + "."});
                return Type::number();
            }
            const StructureInfo *info = nullptr;
            int code = aggregate.kind == TypeKind::Structure ? 19 : 20;
            std::string kind = aggregate.kind == TypeKind::Structure ? "Structure" : "Union";
            if (aggregate.kind == TypeKind::Structure) {
                auto it = structureTable_.find(aggregate.tag);
                if (it != structureTable_.end()) info = &it->second;
            } else {
                auto it = unionTable_.find(aggregate.tag);
                if (it != unionTable_.end()) info = &it->second;
            }
            if (!info || !info->complete) {
                diags.push_back({code, line, kind + " \"" + aggregate.tag +
                                            "\" is incomplete here, so its members are not available."});
                return Type::number();
            }
            const AggregateFieldInfo *field = findAggregateField(base, node.name);
            if (!field) {
                diags.push_back({code, line, kind + " \"" + aggregate.tag + "\" has no member \"" + node.name + "\"."});
                return Type::number();
            }
            if (aggregate.qualifiers.isAtomic) {
                diags.push_back({24, line, "Member access on atomic " + kind + " \"" + aggregate.tag +
                                           "\" is undefined in C; use whole-object atomic operations instead."});
            }
            if (field->bitWidth && analysis_) analysis_->bitFieldExprs.insert(e);
            return memberTypeWithAggregateQualifiers(field->type, aggregate.qualifiers);
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
            if (isList(first) || first.isPointer() || first.kind == TypeKind::Nullptr ||
                first.isArray() || first.isAggregate()) {
                diags.push_back({9, line, "Lists can't contain other lists, native pointers, null pointer values, arrays, or aggregates. Use numbers, decimals, or strings as list items."});
                first = Type::number();
            }
            for (size_t i = 1; i < node.items.size(); ++i) {
                Type item = inferExpr(node.items[i], line, diags);
                if (isList(item) || item.isPointer() || item.kind == TypeKind::Nullptr ||
                    item.isArray() || item.isAggregate() || item != first) {
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
            bool equality = node.op == BinOp::Eq || node.op == BinOp::Ne;
            bool relational = node.op == BinOp::Gt || node.op == BinOp::Lt ||
                              node.op == BinOp::Ge || node.op == BinOp::Le;
            if (relational && (lhsValue.kind == TypeKind::Complex || rhsValue.kind == TypeKind::Complex)) {
                diags.push_back({4, line, "Complex values support equality comparison but not relational ordering."});
                return Type::integer(IntegerRank::Int);
            }
            if (relational && (lhsValue.kind == TypeKind::Nullptr || rhsValue.kind == TypeKind::Nullptr)) {
                diags.push_back({4, line,
                    "Relational comparison cannot use null pointer values; C23 permits nullptr_t only in equality comparisons."});
                return Type::integer(IntegerRank::Int);
            }
            if (equality && lhsPointer && isNullPointerLike(rhsValue, node.rhs)) return Type::integer(IntegerRank::Int);
            if (equality && rhsPointer && isNullPointerLike(lhsValue, node.lhs)) return Type::integer(IntegerRank::Int);
            if (equality && lhsValue.kind == TypeKind::Nullptr && isNullPointerConstantExpr(node.rhs)) return Type::integer(IntegerRank::Int);
            if (equality && rhsValue.kind == TypeKind::Nullptr && isNullPointerConstantExpr(node.lhs)) return Type::integer(IntegerRank::Int);
            if (equality && lhsValue.kind == TypeKind::Nullptr && rhsValue.kind == TypeKind::Nullptr) return Type::integer(IntegerRank::Int);
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
                if (equality && lhsPointer && rhsPointer && pointersComparable(lhsValue, rhsValue)) return Type::number();
                if (relational && lhsPointer && rhsPointer && hasCompletePointee(lhsValue) && hasCompletePointee(rhsValue) &&
                    lhsValue.elementType && rhsValue.elementType && *lhsValue.elementType == *rhsValue.elementType) return Type::number();
                diags.push_back({16, line, "This pointer operation needs compatible object-pointer operands."});
                return Type::number();
            }
            if (node.op == BinOp::Add) {
                bool lhsStr = lhs.kind == TypeKind::String;
                bool rhsStr = rhs.kind == TypeKind::String;
                bool lhsNum = isArithmeticScalar(lhsValue);
                bool rhsNum = isArithmeticScalar(rhsValue);
                if (!((lhsStr && rhsStr) || (lhsNum && rhsNum) || (lhsStr && rhsNum) || (lhsNum && rhsStr))) {
                    diags.push_back({2, line, "I can't add a " + typeToString(lhs) + " to a " + typeToString(rhs) + "."});
                }
                if (lhsStr || rhsStr) return Type::string();
                return usualArithmeticConversion(lhsValue, rhsValue);
            }
            if (node.op == BinOp::Sub || node.op == BinOp::Mul || node.op == BinOp::Div) {
                if (!isArithmeticScalar(lhsValue) || !isArithmeticScalar(rhsValue)) {
                    diags.push_back({2, line, "I can't do arithmetic on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be arithmetic scalars."});
                    return Type::number();
                }
                return usualArithmeticConversion(lhsValue, rhsValue);
            }
            if (node.op == BinOp::Mod) {
                if (!isBitwiseIntegral(lhsValue) || !isBitwiseIntegral(rhsValue)) {
                    diags.push_back({2, line, "Modulo needs integer operands after C value conversion, not " +
                                             typeToString(lhs) + " and " + typeToString(rhs) + "."});
                    return Type::number();
                }
                return usualIntegerConversion(lhsValue, rhsValue);
            }
            if (node.op == BinOp::BitAnd || node.op == BinOp::BitXor || node.op == BinOp::BitOr) {
                if (!isBitwiseIntegral(lhsValue) || !isBitwiseIntegral(rhsValue)) {
                    diags.push_back({25, line, "Bitwise operators need integer operands, not " +
                                              typeToString(lhs) + " and " + typeToString(rhs) + "."});
                    return Type::number();
                }
                return usualIntegerConversion(lhsValue, rhsValue);
            }
            if (node.op == BinOp::ShiftLeft || node.op == BinOp::ShiftRight) {
                if (!isBitwiseIntegral(lhsValue) || !isBitwiseIntegral(rhsValue)) {
                    diags.push_back({25, line, "Shift operators need integer operands, not " +
                                              typeToString(lhs) + " and " + typeToString(rhs) + "."});
                    return Type::number();
                }
                return integerPromotion(lhsValue);
            }
            if (node.op == BinOp::And || node.op == BinOp::Or) {
                if (!isConditionalScalar(lhsValue) || !isConditionalScalar(rhsValue)) {
                    diags.push_back({2, line, "I can't do logical " + std::string(node.op == BinOp::And ? "and" : "or") +
                                             " on a " + typeToString(lhs) + " and a " + typeToString(rhs) + ". Both sides must be C scalar values."});
                }
                return Type::integer(IntegerRank::Int);
            }
            if (isList(lhsValue) || isList(rhsValue) ||
                (lhsValue != rhsValue && !(isArithmeticScalar(lhsValue) && isArithmeticScalar(rhsValue)))) {
                diags.push_back({4, line, "I can't compare a " + typeToString(lhs) + " with a " + typeToString(rhs) +
                                         ". Both sides must be comparable scalar values."});
            }
            return Type::integer(IntegerRank::Int);
        }
        else if constexpr (std::is_same_v<T, UnaryExpr>) {
            Type rhs = inferExpr(node.rhs, line, diags);
            Type value = decayArray(rhs);
            if (node.op == UnaryOp::Neg) {
                if (!isArithmeticScalar(value)) {
                    diags.push_back({2, line, "I can't negate a " + typeToString(rhs) + "."});
                    return Type::number();
                }
                if (isBitwiseIntegral(value)) return integerPromotion(value);
                return value;
            }
            if (node.op == UnaryOp::BitNot) {
                if (!isBitwiseIntegral(value)) {
                    diags.push_back({25, line, "Bitwise not needs an integer operand, not a " + typeToString(rhs) + "."});
                    return Type::number();
                }
                return integerPromotion(value);
            }
            if (!isConditionalScalar(value)) {
                diags.push_back({2, line, "I can't apply not to a " + typeToString(rhs) + ". It must be a C scalar value."});
            }
            return Type::integer(IntegerRank::Int);
        }
        else if constexpr (std::is_same_v<T, CallExpr>) {
            auto found = procTable_.find(node.name);
            if (found == procTable_.end()) {
                diags.push_back({7, line, "I don't know what to do with \"" + node.name +
                                         "\" — it is used here but never defined. Use Procedure to create it first."});
                for (Expr *arg : node.args) inferExpr(arg, line, diags);
                return Type::number();
            }

            const ProcedureSignature &signature = found->second;
            if (node.args.size() != signature.parameterTypes.size()) {
                diags.push_back({8, line, "Call to \"" + node.name + "\" expects " +
                                         std::to_string(signature.parameterTypes.size()) + " arguments but got " +
                                         std::to_string(node.args.size()) + "."});
            }
            for (size_t i = 0; i < node.args.size(); ++i) {
                std::size_t before = diags.size();
                Type argType = inferExpr(node.args[i], line, diags);
                if (signature.nativeTyped) {
                    if (i < signature.parameterTypes.size() && diags.size() == before &&
                        !assignableExprTo(signature.parameterTypes[i], argType, node.args[i])) {
                        diags.push_back({18, line, "Argument " + std::to_string(i + 1) + " to typed Procedure \"" +
                                                 node.name + "\" expects " + typeToString(signature.parameterTypes[i]) +
                                                 " but got " + typeToString(argType) + "."});
                    }
                } else if (argType.isPointer() || argType.kind == TypeKind::Nullptr || argType.isArray()) {
                    diags.push_back({16, line, "Legacy Procedure parameters cannot carry native pointers, null pointer values, or arrays yet."});
                }
            }
            if (signature.nativeTyped && signature.returnType.kind == TypeKind::Void) {
                diags.push_back({18, line, "Typed Procedure \"" + node.name + "\" returns void and cannot be used as an expression."});
                return Type::number();
            }
            return signature.returnType;
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
            validateTypeQualifiers(queried, line, diags);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (!isCompleteObjectType(queried)) {
                diags.push_back({12, line, "I can't ask for the size of " + typeToString(queried) + " because it is not a complete C object type."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, SizeOfExpr>) {
            Type queried = inferExpr(node.operand, line, diags);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (analysis_ && analysis_->bitFieldExprs.count(node.operand)) {
                diags.push_back({23, line, "I can't ask for the size of a bit-field because C does not permit sizeof on a bit-field expression."});
            } else if (!isCompleteObjectType(queried)) {
                diags.push_back({12, line, "I can't ask for the size of a " + typeToString(queried) + " value because it does not currently have complete native C object layout."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, AlignOfTypeExpr>) {
            Type queried = resolveTypeSpec(node.type);
            validateTypeQualifiers(queried, line, diags);
            if (analysis_) analysis_->typeOperands[e] = queried;
            if (!isCompleteObjectType(queried)) {
                diags.push_back({12, line, "I can't ask for the alignment of " + typeToString(queried) + " because it is not a complete C object type."});
            }
            return Type::number();
        }
        else if constexpr (std::is_same_v<T, MathCallExpr>) {
            Type arg = inferExpr(node.arg, line, diags);
            if (node.func == "real" || node.func == "imaginary" || node.func == "magnitude") {
                if (arg.kind != TypeKind::Complex)
                    diags.push_back({2, line, node.func + " part needs a complex decimal, not a " + typeToString(arg) + "."});
            } else if (!isNumeric(arg)) diags.push_back({2, line, "I can't apply " + node.func + " to a " + typeToString(arg) + "."});
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
    auto validateBitField = [&](const StructureField &field, const Type &fieldType) {
        bool valid = true;
        const std::string fieldName = field.name.empty() ? std::string("<unnamed>") : field.name;

        if (fieldType.qualifiers.isAtomic) {
            diags.push_back({24, s->line, "Atomic bit-fields are not supported by the portable C backend; use a non-atomic bit-field or a separate atomic object."});
            valid = false;
        }

        if (fieldType.kind == TypeKind::Enumeration) {
            auto enumeration = enumerationTable_.find(fieldType.tag);
            if (enumeration == enumerationTable_.end()) {
                diags.push_back({23, s->line, "Bit-field \"" + fieldName +
                                          "\" uses unknown Enumeration \"" + fieldType.tag + "\"."});
                return false;
            }
            if (!enumeration->second.complete) {
                diags.push_back({23, s->line, "Bit-field \"" + fieldName +
                                          "\" needs Enumeration \"" + fieldType.tag +
                                          "\" to be complete before it can be used by value."});
                return false;
            }
        }

        auto limit = bitFieldWidthLimit(fieldType);
        if (!limit) {
            diags.push_back({23, s->line, "Bit-field \"" + fieldName +
                                      "\" must use an integer, boolean, or enumeration type supported by the target C compiler."});
            valid = false;
        } else {
            if (!field.name.empty() && *field.bitWidth == 0) {
                diags.push_back({23, s->line, "A named bit-field cannot have width 0; zero width is reserved for unnamed alignment fields."});
                valid = false;
            }
            if (*field.bitWidth > *limit) {
                diags.push_back({23, s->line, "Bit-field \"" + fieldName +
                                          "\" width " + std::to_string(*field.bitWidth) +
                                          " exceeds its target type width of " + std::to_string(*limit) + " bits."});
                valid = false;
            }
        }
        return valid;
    };

    std::visit([&](auto &&node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, SayStmt>) {
            for (Expr *arg : node.args) {
                Type type = inferExpr(arg, s->line, diags);
                if (type.isPointer() || type.kind == TypeKind::Nullptr || type.isArray() || type.isAggregate()) {
                    diags.push_back({16, s->line, "Say does not format native pointers, null pointer values, whole arrays, or aggregates; say a scalar Member, Element, or Value instead."});
                }
            }
        }
        else if constexpr (std::is_same_v<T, StaticAssertStmt>) {
            Type type = inferExpr(node.condition, s->line, diags);
            if (!isIntegralType(type)) {
                diags.push_back({3, s->line, "A static assertion needs a whole-number condition."});
            } else {
                auto value = integerConstantValue(node.condition);
                if (!value) diags.push_back({30, s->line, "A static assertion needs an integer constant expression."});
                else if (*value == 0) diags.push_back({30, s->line, "Static assertion failed."});
            }
        }
        else if constexpr (std::is_same_v<T, RuntimeAssertStmt>) {
            Type type = inferExpr(node.condition, s->line, diags);
            if (!(isArithmeticScalar(type) || type.isPointer() || type.kind == TypeKind::Nullptr))
                diags.push_back({3, s->line, "An assertion needs a scalar condition."});
        }
        else if constexpr (std::is_same_v<T, SetStmt>) {
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (Symbol *existing = findVar(node.name)) {
                bool modifiable = !existing->nativeObject || isModifiableObjectType(existing->type);
                if (existing->nativeObject && !modifiable) {
                    diags.push_back({24, s->line, "I can't Set \"" + node.name +
                                              "\" because its native type " + typeToString(existing->type) +
                                              " is not a modifiable C object type."});
                }
                bool ok = existing->nativeObject ? assignableExprTo(existing->type, exprType, node.expr)
                                                 : existing->type == exprType;
                if (modifiable && !ok) {
                    diags.push_back({3, s->line, "I can't set \"" + node.name + "\", which is a " +
                                             typeToString(existing->type) + ", to a " + typeToString(exprType) + "."});
                }
                if (existing->nativeObject && modifiable && analysis_) analysis_->nativeMutationTargets.insert(s);
            } else if (exprType.isPointer() || exprType.kind == TypeKind::Nullptr ||
                       exprType.isArray() || exprType.isAggregate()) {
                diags.push_back({13, s->line, "Native pointers, null pointer values, arrays, and aggregates need explicit declarations; inferred Set cannot create them."});
            } else {
                declareVar(node.name, exprType, false, s->line, diags);
            }
        }
        else if constexpr (std::is_same_v<T, StructureStmt>) {
            auto found = structureTable_.find(node.name);
            if (found == structureTable_.end() || found->second.complete) return;

            std::vector<AggregateFieldInfo> fields;
            std::unordered_set<std::string> names;
            bool valid = true;
            bool hasFlexible = false;
            std::size_t namedMembers = 0;

            for (std::size_t fieldIndex = 0; fieldIndex < node.fields.size(); ++fieldIndex) {
                const auto &field = node.fields[fieldIndex];
                if (!field.name.empty()) {
                    ++namedMembers;
                    if (!names.insert(field.name).second) {
                        diags.push_back({19, s->line, "Structure \"" + node.name +
                                                  "\" defines member \"" + field.name + "\" more than once."});
                        valid = false;
                        continue;
                    }
                }

                if (field.flexibleArray) {
                    Type elementType = resolveTypeSpec(field.type);
                    if (!validateTypeQualifiers(elementType, s->line, diags)) valid = false;
                    if (field.name.empty()) {
                        diags.push_back({23, s->line, "A flexible array member must have a name."});
                        valid = false;
                    }
                    if (fieldIndex + 1 != node.fields.size()) {
                        diags.push_back({23, s->line, "Flexible member \"" + field.name +
                                                  "\" must be the last member of Structure \"" + node.name + "\"."});
                        valid = false;
                    }
                    if (!isCompleteObjectType(elementType) || containsFlexibleArray(elementType)) {
                        diags.push_back({23, s->line, "Flexible member \"" + field.name +
                                                  "\" needs a complete element type that does not itself contain a flexible array."});
                        valid = false;
                    }
                    fields.push_back(AggregateFieldInfo{
                        field.name, Type::incompleteArrayOf(std::move(elementType)), std::nullopt, true});
                    hasFlexible = true;
                    continue;
                }

                Type fieldType = resolveTypeSpec(field.type);
                if (!validateTypeQualifiers(fieldType, s->line, diags)) valid = false;
                if (field.bitWidth) {
                    if (!validateBitField(field, fieldType)) valid = false;
                    fields.push_back(AggregateFieldInfo{field.name, std::move(fieldType), field.bitWidth, false});
                    continue;
                }

                if (fieldType.kind == TypeKind::Void || fieldType.kind == TypeKind::Function) {
                    diags.push_back({19, s->line, "Structure member \"" + field.name +
                                              "\" needs a complete object type, not " + typeToString(fieldType) + "."});
                    valid = false;
                } else if (!fieldType.isPointer() && !isCompleteObjectType(fieldType)) {
                    diags.push_back({19, s->line, "Structure member \"" + field.name + "\" has incomplete by-value type " +
                                              typeToString(fieldType) + "; use a pointer for recursive or forward references."});
                    valid = false;
                } else if (!fieldType.isPointer() && containsFlexibleArray(fieldType)) {
                    diags.push_back({23, s->line, "Structure member \"" + field.name +
                                              "\" cannot contain a flexible-array structure by value; use a pointer instead."});
                    valid = false;
                }
                fields.push_back(AggregateFieldInfo{field.name, std::move(fieldType), std::nullopt, false});
            }

            if (node.fields.empty()) {
                diags.push_back({19, s->line, "Structure \"" + node.name + "\" needs at least one member in this tranche."});
                valid = false;
            }
            if (hasFlexible && namedMembers < 2) {
                diags.push_back({23, s->line, "Structure \"" + node.name +
                                          "\" needs at least one other named member before its flexible array member."});
                valid = false;
            }

            if (valid) {
                found->second.fields = fields;
                found->second.complete = true;
                found->second.hasFlexibleArray = hasFlexible;
                if (analysis_) {
                    analysis_->structures[node.name] = found->second;
                    analysis_->structureFields[s] = fields;
                }
            }
        }
        else if constexpr (std::is_same_v<T, UnionStmt>) {
            auto found = unionTable_.find(node.name);
            if (found == unionTable_.end() || found->second.complete) return;

            std::vector<AggregateFieldInfo> fields;
            std::unordered_set<std::string> names;
            bool valid = true;
            bool containsFlexible = false;
            for (const auto &field : node.fields) {
                if (!field.name.empty() && !names.insert(field.name).second) {
                    diags.push_back({20, s->line, "Union \"" + node.name +
                                              "\" defines member \"" + field.name + "\" more than once."});
                    valid = false;
                    continue;
                }

                if (field.flexibleArray) {
                    diags.push_back({23, s->line, "Flexible member \"" + field.name +
                                              "\" is not allowed in a Union; C flexible array members are structure-only."});
                    valid = false;
                    Type elementType = resolveTypeSpec(field.type);
                    if (!validateTypeQualifiers(elementType, s->line, diags)) valid = false;
                    fields.push_back(AggregateFieldInfo{
                        field.name, Type::incompleteArrayOf(std::move(elementType)), std::nullopt, true});
                    continue;
                }

                Type fieldType = resolveTypeSpec(field.type);
                if (!validateTypeQualifiers(fieldType, s->line, diags)) valid = false;
                if (field.bitWidth) {
                    if (!validateBitField(field, fieldType)) valid = false;
                    fields.push_back(AggregateFieldInfo{field.name, std::move(fieldType), field.bitWidth, false});
                    continue;
                }

                if (fieldType.kind == TypeKind::Void || fieldType.kind == TypeKind::Function) {
                    diags.push_back({20, s->line, "Union member \"" + field.name +
                                              "\" needs a complete object type, not " + typeToString(fieldType) + "."});
                    valid = false;
                } else if (!fieldType.isPointer() && !isCompleteObjectType(fieldType)) {
                    diags.push_back({20, s->line, "Union member \"" + field.name + "\" has incomplete by-value type " +
                                              typeToString(fieldType) + "; use a pointer for recursive or forward references."});
                    valid = false;
                } else if (!fieldType.isPointer() && containsFlexibleArray(fieldType)) {
                    containsFlexible = true;
                }
                fields.push_back(AggregateFieldInfo{field.name, std::move(fieldType), std::nullopt, false});
            }

            if (node.fields.empty()) {
                diags.push_back({20, s->line, "Union \"" + node.name + "\" needs at least one member in this tranche."});
                valid = false;
            }

            if (valid) {
                found->second.fields = fields;
                found->second.complete = true;
                found->second.hasFlexibleArray = containsFlexible;
                if (analysis_) {
                    analysis_->unions[node.name] = found->second;
                    analysis_->unionFields[s] = fields;
                }
            }
        }
        else if constexpr (std::is_same_v<T, EnumerationStmt>) {
            auto found = enumerationTable_.find(node.name);
            if (found == enumerationTable_.end() || found->second.complete) return;

            std::vector<std::pair<std::string, long>> values;
            std::unordered_set<std::string> names;
            bool valid = true;
            long previous = -1;
            bool havePrevious = false;
            constexpr long minInt = static_cast<long>(std::numeric_limits<int>::min());
            constexpr long maxInt = static_cast<long>(std::numeric_limits<int>::max());

            for (const auto &enumerator : node.enumerators) {
                if (!names.insert(enumerator.name).second) {
                    diags.push_back({22, s->line, "Enumeration \"" + node.name +
                                              "\" defines Enumerator \"" + enumerator.name + "\" more than once."});
                    valid = false;
                    continue;
                }

                long value = 0;
                if (enumerator.explicitValue) {
                    value = *enumerator.explicitValue;
                    if (value < minInt || value > maxInt) {
                        diags.push_back({22, s->line, "Enumerator \"" + enumerator.name + "\" value " +
                                                  std::to_string(value) +
                                                  " is outside the C99-C17 int range supported by this backend tranche."});
                        valid = false;
                    }
                } else if (!havePrevious) {
                    value = 0;
                } else if (previous == maxInt) {
                    diags.push_back({22, s->line, "Implicit Enumerator \"" + enumerator.name +
                                              "\" would exceed the supported C int range."});
                    value = previous;
                    valid = false;
                } else {
                    value = previous + 1;
                }

                values.emplace_back(enumerator.name, value);
                previous = value;
                havePrevious = true;
            }

            if (node.enumerators.empty()) {
                diags.push_back({22, s->line, "Enumeration \"" + node.name + "\" needs at least one Enumerator."});
                valid = false;
            }

            if (valid) {
                found->second.enumerators = values;
                found->second.complete = true;
                if (analysis_) {
                    analysis_->enumerations[node.name] = found->second;
                    analysis_->enumerationValues[s] = values;
                }
            }
        }
        else if constexpr (std::is_same_v<T, NativeDeclStmt>) {
            Type declared = resolveTypeSpec(node.type);
            if (node.type.kind == TypeSpecKind::Auto) {
                if (!node.initializer) {
                    diags.push_back({13, s->line, "A native auto declaration needs an initializer to infer its type."});
                    return;
                }
                declared = inferExpr(node.initializer, s->line, diags);
                if (declared.kind == TypeKind::String || declared.kind == TypeKind::List || declared.kind == TypeKind::Void) {
                    diags.push_back({13, s->line, "C23 auto inference needs a native object type, not " + typeToString(declared) + "."});
                    return;
                }
            }
            if (node.constexprObject) {
                if (!node.initializer || !integerConstantValue(node.initializer)) {
                    diags.push_back({24, s->line, "A constexpr native object needs an integer constant initializer."});
                    return;
                }
                declared.qualifiers.isConst = true;
            }
            if (analysis_) analysis_->declarationTypes[s] = declared;
            if (!validateTypeQualifiers(declared, s->line, diags)) return;
            if (declared.kind == TypeKind::Void) {
                diags.push_back({13, s->line, "I can't Declare \"" + node.name + "\" as void because void is not an object type."});
                return;
            }
            if (!isCompleteObjectType(declared)) {
                if (declared.isArray()) {
                    diags.push_back({17, s->line, "A fixed native array needs a complete object element type."});
                } else {
                    diags.push_back({19, s->line, "Native object \"" + node.name + "\" needs a complete type; " +
                                              typeToString(declared) + " is incomplete here."});
                }
                return;
            }
            bool created = declareVar(node.name, declared, true, s->line, diags);
            if (scopes_.size() == 1 && hasConstSubobject(declared) && node.initializer && !node.constexprObject) {
                diags.push_back({24, s->line, "A top-level constant native object cannot use a runtime PlainSpeak initializer yet; this backend must emit constant initialization at C file scope first."});
                return;
            }
            if (node.aggregateInitializer &&
                (hasConstSubobject(declared) || declared.qualifiers.isAtomic)) {
                diags.push_back({24, s->line, "This aggregate initializer requires post-declaration member stores, which are not valid for constant subobjects or atomic aggregate objects."});
                return;
            }
            if (node.initializer) {
                std::size_t diagnosticCount = diags.size();
                Type init = inferExpr(node.initializer, s->line, diags);
                bool initializerFailed = diags.size() != diagnosticCount;
                if (declared.isArray()) {
                    if (!initializerFailed) {
                        diags.push_back({17, s->line, "Whole-array assignment-style initialization is not supported; use with values or with elements."});
                    }
                } else if (created && !initializerFailed && !assignableExprTo(declared, init, node.initializer)) {
                    diags.push_back({13, s->line, "I can't initialize native object \"" + node.name +
                                              "\" of type " + typeToString(declared) + " with a " + typeToString(init) + "."});
                }
            }

            if (node.aggregateInitializer) {
                const auto &aggregate = *node.aggregateInitializer;
                auto checkValue = [&](const Type &target, Expr *expr, const std::string &where) {
                    std::size_t before = diags.size();
                    Type source = inferExpr(expr, s->line, diags);
                    if (target.isArray()) {
                        if (diags.size() == before) {
                            diags.push_back({21, s->line, where + " is an array and needs its own aggregate initializer; nested aggregate initializers are not in this tranche."});
                        }
                    } else if (diags.size() == before && !assignableExprTo(target, source, expr)) {
                        diags.push_back({21, s->line, "I can't initialize " + where + ", which is a " +
                                                  typeToString(target) + ", with a " + typeToString(source) + "."});
                    }
                };

                if (aggregate.kind == AggregateInitKind::Positional) {
                    if (declared.isArray() && declared.elementType && declared.arrayBound) {
                        if (aggregate.entries.size() > *declared.arrayBound) {
                            diags.push_back({21, s->line, "Array \"" + node.name + "\" has length " +
                                                      std::to_string(*declared.arrayBound) + " but received " +
                                                      std::to_string(aggregate.entries.size()) + " positional initializers."});
                        }
                        std::size_t count = std::min(aggregate.entries.size(), *declared.arrayBound);
                        for (std::size_t i = 0; i < count; ++i) {
                            checkValue(*declared.elementType, aggregate.entries[i].expr,
                                       "element " + std::to_string(i) + " of \"" + node.name + "\"");
                        }
                    } else if (declared.kind == TypeKind::Structure || declared.kind == TypeKind::Union) {
                        const StructureInfo *info = nullptr;
                        if (declared.kind == TypeKind::Structure) {
                            auto it = structureTable_.find(declared.tag);
                            if (it != structureTable_.end()) info = &it->second;
                        } else {
                            auto it = unionTable_.find(declared.tag);
                            if (it != unionTable_.end()) info = &it->second;
                        }
                        if (info && info->complete) {
                            std::vector<const AggregateFieldInfo *> positionalFields;
                            for (const auto &field : info->fields) {
                                if (!field.name.empty() && !field.flexibleArray) positionalFields.push_back(&field);
                            }
                            std::size_t allowed = declared.kind == TypeKind::Union
                                                    ? std::min<std::size_t>(1, positionalFields.size())
                                                    : positionalFields.size();
                            if (aggregate.entries.size() > allowed) {
                                diags.push_back({21, s->line, (declared.kind == TypeKind::Union ? "Union" : "Structure") +
                                                          std::string(" \"") + declared.tag + "\" accepts at most " +
                                                          std::to_string(allowed) + " positional initializer" +
                                                          (allowed == 1 ? "" : "s") + "."});
                            }
                            std::size_t count = std::min(aggregate.entries.size(), allowed);
                            for (std::size_t i = 0; i < count; ++i) {
                                checkValue(positionalFields[i]->type, aggregate.entries[i].expr,
                                           "member \"" + positionalFields[i]->name + "\" of \"" + node.name + "\"");
                            }
                        }
                    } else {
                        diags.push_back({21, s->line, "with values requires an array, structure, or union target, not " +
                                                  typeToString(declared) + "."});
                        for (const auto &entry : aggregate.entries) inferExpr(entry.expr, s->line, diags);
                    }
                } else if (aggregate.kind == AggregateInitKind::Members) {
                    if (declared.kind == TypeKind::Union && aggregate.entries.size() > 1) {
                        diags.push_back({21, s->line, "A union initializer selects exactly one member; \"" + node.name +
                                                  "\" received " + std::to_string(aggregate.entries.size()) + " member designators."});
                    }
                    if (declared.kind != TypeKind::Structure && declared.kind != TypeKind::Union) {
                        diags.push_back({21, s->line, "with members requires a structure or union target, not " +
                                                  typeToString(declared) + "."});
                        for (const auto &entry : aggregate.entries) inferExpr(entry.expr, s->line, diags);
                    } else {
                        std::unordered_set<std::string> used;
                        for (const auto &entry : aggregate.entries) {
                            if (!used.insert(entry.memberName).second) {
                                diags.push_back({21, s->line, "Member designator \"" + entry.memberName +
                                                          "\" appears more than once in the initializer for \"" + node.name + "\"."});
                                inferExpr(entry.expr, s->line, diags);
                                continue;
                            }
                            Type aggregateType = declared;
                            const AggregateFieldInfo *field = findAggregateField(aggregateType, entry.memberName);
                            if (!field) {
                                diags.push_back({21, s->line, (declared.kind == TypeKind::Structure ? "Structure" : "Union") +
                                                          std::string(" \"") + declared.tag + "\" has no member \"" +
                                                          entry.memberName + "\" to initialize."});
                                inferExpr(entry.expr, s->line, diags);
                                continue;
                            }
                            if (field->flexibleArray) {
                                diags.push_back({21, s->line, "Flexible member \"" + entry.memberName +
                                                          "\" is not an initializer target; C flexible array storage is outside sizeof the structure."});
                                inferExpr(entry.expr, s->line, diags);
                                continue;
                            }
                            checkValue(field->type, entry.expr, "member \"" + entry.memberName + "\" of \"" + node.name + "\"");
                        }
                    }
                } else {
                    if (!declared.isArray() || !declared.elementType || !declared.arrayBound) {
                        diags.push_back({21, s->line, "with elements requires a fixed native array target, not " +
                                                  typeToString(declared) + "."});
                        for (const auto &entry : aggregate.entries) inferExpr(entry.expr, s->line, diags);
                    } else {
                        std::unordered_set<std::size_t> used;
                        for (const auto &entry : aggregate.entries) {
                            if (entry.elementIndex >= *declared.arrayBound) {
                                diags.push_back({21, s->line, "Element designator " + std::to_string(entry.elementIndex) +
                                                          " is outside array \"" + node.name + "\" of length " +
                                                          std::to_string(*declared.arrayBound) + "."});
                                inferExpr(entry.expr, s->line, diags);
                                continue;
                            }
                            if (!used.insert(entry.elementIndex).second) {
                                diags.push_back({21, s->line, "Element designator " + std::to_string(entry.elementIndex) +
                                                          " appears more than once in the initializer for \"" + node.name + "\"."});
                                inferExpr(entry.expr, s->line, diags);
                                continue;
                            }
                            checkValue(*declared.elementType, entry.expr,
                                       "element " + std::to_string(entry.elementIndex) + " of \"" + node.name + "\"");
                        }
                    }
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
            } else if (!isModifiableObjectType(*pointer.elementType)) {
                diags.push_back({24, s->line, "I can't store through " + typeToString(pointer) +
                                          " because its pointed-to object type is not modifiable."});
            } else if (!assignableExprTo(*pointer.elementType, value, node.expr)) {
                diags.push_back({13, s->line, "I can't store a " + typeToString(value) + " through a " + typeToString(pointer) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, StoreMemberStmt>) {
            Type base = inferExpr(node.base, s->line, diags);
            Type value = inferExpr(node.expr, s->line, diags);
            Type aggregate = base;
            if (aggregate.isPointer() && aggregate.elementType) aggregate = *aggregate.elementType;
            if (aggregate.kind != TypeKind::Structure && aggregate.kind != TypeKind::Union) {
                diags.push_back({19, s->line, "Set member " + node.name + " needs a structure/union object or pointer target, not a " +
                                           typeToString(base) + "."});
            } else {
                const StructureInfo *info = nullptr;
                int code = aggregate.kind == TypeKind::Structure ? 19 : 20;
                std::string kind = aggregate.kind == TypeKind::Structure ? "Structure" : "Union";
                if (aggregate.kind == TypeKind::Structure) {
                    auto it = structureTable_.find(aggregate.tag);
                    if (it != structureTable_.end()) info = &it->second;
                } else {
                    auto it = unionTable_.find(aggregate.tag);
                    if (it != unionTable_.end()) info = &it->second;
                }
                if (!info || !info->complete) {
                    diags.push_back({code, s->line, kind + " \"" + aggregate.tag +
                                                "\" is incomplete here, so its members cannot be stored."});
                } else if (aggregate.qualifiers.isAtomic) {
                    diags.push_back({24, s->line, "Member access on atomic " + kind + " \"" + aggregate.tag +
                                              "\" is undefined in C; use whole-object atomic operations instead."});
                } else if (const AggregateFieldInfo *field = findAggregateField(base, node.name)) {
                    Type effectiveField = memberTypeWithAggregateQualifiers(field->type, aggregate.qualifiers);
                    if (effectiveField.isArray()) {
                        diags.push_back({code, s->line, "Whole-array aggregate member assignment is not implemented yet."});
                    } else if (!isModifiableObjectType(effectiveField)) {
                        diags.push_back({24, s->line, "I can't store in member \"" + node.name +
                                                  "\" because its effective type " + typeToString(effectiveField) +
                                                  " is not a modifiable C object type."});
                    } else if (!assignableExprTo(effectiveField, value, node.expr)) {
                        diags.push_back({code, s->line, "I can't store a " + typeToString(value) + " in member \"" +
                                                   node.name + "\", which is a " + typeToString(effectiveField) + "."});
                    }
                } else {
                    diags.push_back({code, s->line, kind + " \"" + aggregate.tag + "\" has no member \"" + node.name + "\"."});
                }
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
            } else if (!isModifiableObjectType(*base.elementType)) {
                diags.push_back({24, s->line, "I can't store an element through " + typeToString(base) +
                                          " because its element type is not modifiable."});
            } else if (!assignableExprTo(*base.elementType, value, node.expr)) {
                diags.push_back({13, s->line, "I can't store a " + typeToString(value) + " as an element of " + typeToString(base) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, AddStmt> || std::is_same_v<T, SubStmt>) {
            auto [symbol, found] = lookupVar(node.varName, s->line, diags);
            Type exprType = inferExpr(node.expr, s->line, diags);
            if (found && symbol.nativeObject) {
                bool modifiable = isModifiableObjectType(symbol.type);
                if (!modifiable) {
                    diags.push_back({24, s->line, "I can't change \"" + node.varName +
                                              "\" with Add/Subtract because its native type " +
                                              typeToString(symbol.type) + " is not modifiable."});
                } else if (symbol.type.isPointer()) {
                    if (!hasCompletePointee(symbol.type) || !isIntegralType(exprType)) {
                        diags.push_back({16, s->line, "Changing a pointer with Add/Subtract needs a complete object pointer and an integer offset."});
                    }
                } else if (!isArithmeticScalar(symbol.type) || !isArithmeticScalar(exprType)) {
                    diags.push_back({3, s->line, "I can only change native arithmetic objects or object pointers with Add/Subtract; \"" +
                                             node.varName + "\" is a " + typeToString(symbol.type) + "."});
                }
                if (modifiable && analysis_) analysis_->nativeMutationTargets.insert(s);
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
        else if constexpr (std::is_same_v<T, BreakStmt>) {
            if (breakableDepth_ == 0) {
                diags.push_back({29, s->line, "Break can only appear inside a loop or another C breakable construct."});
            }
        }
        else if constexpr (std::is_same_v<T, ContinueStmt>) {
            if (loopDepth_ == 0) {
                diags.push_back({29, s->line, "Continue can only appear inside a loop."});
            }
        }
        else if constexpr (std::is_same_v<T, GotoStmt>) {
            gotoTargets_[node.label] = s->line;
        }
        else if constexpr (std::is_same_v<T, LabelStmt>) {
            if (labels_.count(node.name)) {
                diags.push_back({32, s->line, "Label \"" + node.name +
                                          "\" is already defined in this function."});
            } else {
                labels_[node.name] = s->line;
            }
        }
        else if constexpr (std::is_same_v<T, RepeatStmt>) {
            Type countType = inferExpr(node.count, s->line, diags);
            if (countType != Type::number()) {
                diags.push_back({5, s->line, "Repeat needs a whole number of times, not a " + typeToString(countType) + "."});
            }
            ++loopDepth_;
            ++breakableDepth_;
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
            --breakableDepth_;
            --loopDepth_;
        }
        else if constexpr (std::is_same_v<T, IfStmt>) {
            Type cond = inferExpr(node.cond, s->line, diags);
            (void)cond;
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
            if (!isConditionalScalar(condType)) {
                diags.push_back({5, s->line, "While needs a C scalar condition, not a " + typeToString(condType) + "."});
            }
            ++loopDepth_;
            ++breakableDepth_;
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
            --breakableDepth_;
            --loopDepth_;
        }
        else if constexpr (std::is_same_v<T, DoWhileStmt>) {
            Type condType = inferExpr(node.cond, s->line, diags);
            if (!isConditionalScalar(condType)) {
                diags.push_back({5, s->line, "Do while needs a C scalar condition, not a " + typeToString(condType) + "."});
            }
            ++loopDepth_;
            ++breakableDepth_;
            enterScope();
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
            --breakableDepth_;
            --loopDepth_;
        }
        else if constexpr (std::is_same_v<T, ForEachStmt>) {
            Type listType = inferExpr(node.list, s->line, diags);
            Type itemType = Type::number();
            if (!isList(listType)) {
                diags.push_back({10, s->line, "For each needs a list to walk through, not a " + typeToString(listType) + "."});
            } else {
                itemType = listElementType(listType);
            }
            ++loopDepth_;
            ++breakableDepth_;
            enterScope();
            declareVar(node.itemName, itemType, false, s->line, diags);
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
            --breakableDepth_;
            --loopDepth_;
        }
        else if constexpr (std::is_same_v<T, ForStmt>) {
            std::size_t before = diags.size();
            Type fromType = inferExpr(node.from, s->line, diags);
            Type toType = inferExpr(node.to, s->line, diags);
            if (diags.size() == before && (!isIntegralType(fromType) || !isIntegralType(toType))) {
                diags.push_back({5, s->line, "For needs whole-number start and end bounds, not a " +
                                            typeToString(fromType) + " and a " + typeToString(toType) + "."});
            }
            ++loopDepth_;
            ++breakableDepth_;
            enterScope();
            declareVar(node.varName, Type::number(), true, s->line, diags);
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            leaveScope();
            --breakableDepth_;
            --loopDepth_;
        }
        else if constexpr (std::is_same_v<T, SwitchStmt>) {
            std::size_t before = diags.size();
            Type condType = inferExpr(node.cond, s->line, diags);
            if (diags.size() == before && !isIntegralType(condType)) {
                diags.push_back({30, s->line, "Switch needs a whole-number value to test, not a " +
                                            typeToString(condType) + "."});
            }
            ++breakableDepth_;
            // C cases share one block scope, so variables declared in one When
            // clause stay visible (and collide consistently) across the switch.
            enterScope();
            std::unordered_set<long> seen;
            bool seenDefault = false;
            for (const auto &c : node.cases) {
                if (!c.value) {
                    if (seenDefault) {
                        diags.push_back({30, s->line, "A Switch can have only one Otherwise clause."});
                    }
                    seenDefault = true;
                } else {
                    auto constant = integerConstantValue(c.value);
                    if (!constant) {
                        diags.push_back({30, s->line, "A When clause needs a whole-number constant to match; a runtime value cannot label a C switch case."});
                    } else {
                        if (analysis_) analysis_->switchCaseValues[c.value] = *constant;
                        if (!seen.insert(*constant).second) {
                            diags.push_back({30, s->line, "Switch has more than one When clause for the value " +
                                                        std::to_string(*constant) + "."});
                        }
                    }
                }
                for (Stmt *inner : c.body) checkStmt(inner, diags);
            }
            leaveScope();
            --breakableDepth_;
        }
        else if constexpr (std::is_same_v<T, ProcedureStmt>) {
            auto signatureIt = procTable_.find(node.name);
            if (signatureIt == procTable_.end()) return;
            ProcedureSignature previous = currentProcedure_.value_or(ProcedureSignature{});
            bool hadPrevious = currentProcedure_.has_value();
            int previousLoopDepth = loopDepth_;
            int previousBreakableDepth = breakableDepth_;
            auto previousLabels = std::move(labels_);
            auto previousGotos = std::move(gotoTargets_);
            currentProcedure_ = signatureIt->second;
            loopDepth_ = 0;
            breakableDepth_ = 0;

            enterScope();
            for (size_t i = 0; i < node.params.size(); ++i) {
                Type paramType = i < signatureIt->second.parameterTypes.size()
                               ? signatureIt->second.parameterTypes[i] : Type::number();
                declareVar(node.params[i].name, paramType, signatureIt->second.nativeTyped, s->line, diags);
            }
            for (Stmt *inner : node.body) checkStmt(inner, diags);
            validateLabels(diags);

            if (signatureIt->second.nativeTyped &&
                signatureIt->second.returnType.kind != TypeKind::Void) {
                if (ReturnPathChecker::endIsReachable(node.body)) {
                    diags.push_back({18, s->line, "Typed Procedure \"" + node.name +
                                              "\" can reach its end without a Return on some path; every path must return a value."});
                }
            }

            leaveScope();
            loopDepth_ = previousLoopDepth;
            breakableDepth_ = previousBreakableDepth;
            labels_ = std::move(previousLabels);
            gotoTargets_ = std::move(previousGotos);
            if (hadPrevious) currentProcedure_ = previous;
            else currentProcedure_.reset();
        }
        else if constexpr (std::is_same_v<T, CallStmt>) {
            auto found = procTable_.find(node.name);
            if (found == procTable_.end()) {
                diags.push_back({7, s->line, "I don't know what to do with \"" + node.name +
                                           "\" — it is used here but never defined. Use Procedure to create it first."});
                for (Expr *arg : node.args) inferExpr(arg, s->line, diags);
            } else {
                const ProcedureSignature &signature = found->second;
                if (node.args.size() != signature.parameterTypes.size()) {
                    diags.push_back({8, s->line, "Call to \"" + node.name + "\" expects " +
                                             std::to_string(signature.parameterTypes.size()) + " arguments but got " +
                                             std::to_string(node.args.size()) + "."});
                }
                for (size_t i = 0; i < node.args.size(); ++i) {
                    std::size_t before = diags.size();
                    Type argType = inferExpr(node.args[i], s->line, diags);
                    if (signature.nativeTyped) {
                        if (i < signature.parameterTypes.size() && diags.size() == before &&
                            !assignableExprTo(signature.parameterTypes[i], argType, node.args[i])) {
                            diags.push_back({18, s->line, "Argument " + std::to_string(i + 1) + " to typed Procedure \"" +
                                                     node.name + "\" expects " + typeToString(signature.parameterTypes[i]) +
                                                     " but got " + typeToString(argType) + "."});
                        }
                    } else if (argType.isPointer() || argType.kind == TypeKind::Nullptr || argType.isArray()) {
                        diags.push_back({16, s->line, "Legacy Procedure parameters cannot carry native pointers, null pointer values, or arrays yet."});
                    }
                }
            }
        }
        else if constexpr (std::is_same_v<T, ReturnStmt>) {
            if (!currentProcedure_) {
                diags.push_back({18, s->line, "Return can only appear inside a Procedure."});
                if (node.expr) inferExpr(node.expr, s->line, diags);
                return;
            }

            const ProcedureSignature &signature = *currentProcedure_;
            if (!signature.nativeTyped) {
                if (!node.expr) {
                    diags.push_back({18, s->line, "A legacy Procedure Return needs a value; use an explicitly typed Procedure returning void for a bare Return."});
                    return;
                }
                Type type = inferExpr(node.expr, s->line, diags);
                if (type.isPointer() || type.kind == TypeKind::Nullptr || type.isArray()) {
                    diags.push_back({16, s->line, "Legacy Procedures cannot return native pointers, null pointer values, or arrays yet."});
                }
                return;
            }

            if (signature.returnType.kind == TypeKind::Void) {
                if (node.expr) {
                    inferExpr(node.expr, s->line, diags);
                    diags.push_back({18, s->line, "A Procedure returning void must use bare Return. or fall through; it cannot return a value."});
                }
                return;
            }

            if (!node.expr) {
                diags.push_back({18, s->line, "This typed Procedure returns " + typeToString(signature.returnType) +
                                          ", so Return needs a value."});
                return;
            }

            std::size_t before = diags.size();
            Type returned = inferExpr(node.expr, s->line, diags);
            if (diags.size() == before && !assignableExprTo(signature.returnType, returned, node.expr)) {
                diags.push_back({18, s->line, "This typed Procedure returns " + typeToString(signature.returnType) +
                                          " but Return provides " + typeToString(returned) + "."});
            }
        }
        else if constexpr (std::is_same_v<T, CommentStmt>) {
        }
    }, s->node);
}
