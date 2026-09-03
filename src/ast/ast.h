#pragma once
#include <deque>
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
enum class BinOp { Add, Sub, Mul, Div, Mod, Gt, Lt, Eq, Ne, Ge, Le, And, Or };
enum class UnaryOp { Not, Neg };
struct UnaryExpr     { UnaryOp op; Expr *rhs; };
struct LengthExpr    { Expr *operand; };
struct MathCallExpr  { std::string func; Expr *arg; };
struct CallExpr      { std::string name; std::vector<Expr *> args; };
struct PowExpr       { Expr *base; Expr *exp; };
struct BinaryExpr    { BinOp op; Expr *lhs; Expr *rhs; };
struct ListExpr      { std::vector<Expr *> items; };
struct EmptyListExpr { ListElementKind elementKind; };
struct ItemExpr      { Expr *index; Expr *list; };

using ExprNode = std::variant<IntLit, BoolLit, FloatLit, StringLit, VarRef, LengthExpr, MathCallExpr, CallExpr, PowExpr, BinaryExpr, UnaryExpr, ListExpr, EmptyListExpr, ItemExpr>;
struct Expr { ExprNode node; int line; };

struct SayStmt       { Expr *expr; };
struct SetStmt       { std::string name; Expr *expr; };
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
struct ProcedureStmt { std::string name; std::vector<std::string> params; std::vector<Stmt *> body; };
struct ReturnStmt    { Expr *expr; };
struct CommentStmt   { std::string text; };

using StmtNode = std::variant<SayStmt, SetStmt, AddStmt, SubStmt, ReadStmt, ReadFloatStmt, AppendStmt, ReplaceItemStmt, RemoveItemStmt, RepeatStmt, IfStmt, WhileStmt, ForEachStmt, CallStmt, ProcedureStmt, ReturnStmt, CommentStmt>;
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
