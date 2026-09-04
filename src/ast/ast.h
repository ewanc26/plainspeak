#pragma once
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// AST nodes are plain structs held in std::variant sum types. Nodes are
// owned by an Arena (below) and referenced by raw pointer — one
// compilation unit = one arena = freed in one shot. See AGENTS.md §6.

struct Expr;
struct Stmt;

struct IntLit    { long value; };
struct BoolLit   { bool value; };
struct FloatLit  { double value; };
struct StringLit { std::string value; };
struct VarRef    { std::string name; };

enum class ListElementKind { Number, Decimal, String };

// AST-level type spellings are deliberately independent from semantic Type.
// Sema resolves these source spellings into the structural C-capable model in
// src/sema/type.h. Pointer is recursive so prose can spell pointer-to-pointer
// types without duplicating C's declarator grammar.
enum class TypeSpecKind {
    Void,
    Boolean,
    Character,
    SignedCharacter,
    UnsignedCharacter,
    ShortInteger,
    UnsignedShortInteger,
    Integer,
    UnsignedInteger,
    LongInteger,
    UnsignedLongInteger,
    LongLongInteger,
    UnsignedLongLongInteger,
    Float,
    Decimal,
    LongDecimal,
    Pointer,
    Array,
    Structure,
    Union,
    Enumeration
};

struct TypeSpecQualifiers {
    bool isConst = false;
    bool isVolatile = false;
    bool isRestrict = false;
    bool isAtomic = false;
};

struct TypeSpec {
    TypeSpecKind kind;
    std::shared_ptr<TypeSpec> pointee{};
    std::size_t arrayBound = 0;
    std::string tag{};
    TypeSpecQualifiers qualifiers{};
};

enum class BinOp { Add, Sub, Mul, Div, Mod, Gt, Lt, Eq, Ne, Ge, Le, And, Or };
enum class UnaryOp { Not, Neg };
struct UnaryExpr       { UnaryOp op; Expr *rhs; };
struct LengthExpr      { Expr *operand; };
struct SizeOfTypeExpr  { TypeSpec type; };
struct SizeOfExpr      { Expr *operand; };
struct AlignOfTypeExpr { TypeSpec type; };
struct AddressOfExpr   { std::string name; };
struct DerefExpr       { Expr *pointer; };
struct ElementExpr     { Expr *index; Expr *base; };
struct MemberExpr      { std::string name; Expr *base; };
struct EnumeratorExpr  { std::string name; std::string enumeration; };
struct MathCallExpr    { std::string func; Expr *arg; };
struct CallExpr        { std::string name; std::vector<Expr *> args; };
struct PowExpr         { Expr *base; Expr *exp; };
struct BinaryExpr      { BinOp op; Expr *lhs; Expr *rhs; };
struct ListExpr        { std::vector<Expr *> items; };
struct EmptyListExpr   { ListElementKind elementKind; };
struct ItemExpr        { Expr *index; Expr *list; };

using ExprNode = std::variant<IntLit, BoolLit, FloatLit, StringLit, VarRef,
                              LengthExpr, SizeOfTypeExpr, SizeOfExpr,
                              AlignOfTypeExpr, AddressOfExpr, DerefExpr,
                              ElementExpr, MemberExpr, EnumeratorExpr, MathCallExpr, CallExpr, PowExpr, BinaryExpr,
                              UnaryExpr, ListExpr, EmptyListExpr, ItemExpr>;
struct Expr { ExprNode node; int line; };

struct SayStmt       { Expr *expr; };
struct SetStmt       { std::string name; Expr *expr; };
enum class AggregateInitKind { Positional, Members, Elements };
struct AggregateInitEntry {
    std::string memberName{};
    std::size_t elementIndex = 0;
    Expr *expr = nullptr;
};
struct AggregateInitializer {
    AggregateInitKind kind = AggregateInitKind::Positional;
    std::vector<AggregateInitEntry> entries;
};
struct NativeDeclStmt {
    std::string name;
    TypeSpec type;
    Expr *initializer;
    std::optional<AggregateInitializer> aggregateInitializer;
};
struct StoreThroughStmt { Expr *pointer; Expr *expr; };
struct StoreElementStmt { Expr *index; Expr *base; Expr *expr; };
struct StoreMemberStmt { std::string name; Expr *base; Expr *expr; };
struct StructureField {
    std::string name;
    TypeSpec type;
    std::optional<std::size_t> bitWidth;
    bool flexibleArray = false;
};
struct StructureStmt { std::string name; std::vector<StructureField> fields; };
struct UnionStmt { std::string name; std::vector<StructureField> fields; };
struct EnumeratorDef { std::string name; std::optional<long> explicitValue; };
struct EnumerationStmt { std::string name; std::vector<EnumeratorDef> enumerators; };
struct AddStmt       { Expr *expr; std::string varName; };
struct SubStmt       { Expr *expr; std::string varName; };
struct ReadStmt      { std::string varName; };
struct ReadFloatStmt { std::string varName; };
struct AppendStmt    { Expr *expr; std::string varName; };
struct ReplaceItemStmt { Expr *index; std::string varName; Expr *expr; };
struct RemoveItemStmt  { Expr *index; std::string varName; };
struct RepeatStmt    { Expr *count; std::vector<Stmt *> body; };
struct IfStmt        { Expr *cond; std::vector<Stmt *> thenBody; std::vector<Stmt *> elseBody; };
struct WhileStmt     { Expr *cond; std::vector<Stmt *> body; };
struct ForEachStmt   { std::string itemName; Expr *list; std::vector<Stmt *> body; };
struct CallStmt      { std::string name; std::vector<Expr *> args; };
struct ProcedureParam { std::string name; std::optional<TypeSpec> type; };
struct ProcedureStmt {
    std::string name;
    std::vector<ProcedureParam> params;
    std::optional<TypeSpec> returnType;
    std::vector<Stmt *> body;
};
struct ReturnStmt    { Expr *expr; };
struct CommentStmt   { std::string text; };

using StmtNode = std::variant<SayStmt, SetStmt, NativeDeclStmt, StructureStmt, UnionStmt, EnumerationStmt,
                              StoreThroughStmt, StoreElementStmt, StoreMemberStmt, AddStmt, SubStmt, ReadStmt,
                              ReadFloatStmt, AppendStmt, ReplaceItemStmt,
                              RemoveItemStmt, RepeatStmt, IfStmt, WhileStmt,
                              ForEachStmt, CallStmt, ProcedureStmt, ReturnStmt,
                              CommentStmt>;
struct Stmt { StmtNode node; int line; };

// Owns every Expr/Stmt produced while parsing one source file. deque
// guarantees stable element addresses across push_back, so raw pointers
// into it stay valid for the arena's lifetime.
class Arena {
public:
    Expr *makeExpr(ExprNode node, int line) {
        exprs_.push_back(Expr{std::move(node), line});
        return &exprs_.back();
    }
    Stmt *makeStmt(StmtNode node, int line) {
        stmts_.push_back(Stmt{std::move(node), line});
        return &stmts_.back();
    }

private:
    std::deque<Expr> exprs_;
    std::deque<Stmt> stmts_;
};
