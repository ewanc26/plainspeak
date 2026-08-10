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
struct StringLit { std::string value; };
struct VarRef    { std::string name; };

enum class BinOp { Add, Sub, Mul, Div, Gt, Lt, Eq };
struct BinaryExpr { BinOp op; Expr *lhs; Expr *rhs; };

using ExprNode = std::variant<IntLit, StringLit, VarRef, BinaryExpr>;
struct Expr { ExprNode node; int line; };

struct SayStmt   { Expr *expr; };
struct SetStmt   { std::string name; Expr *expr; };
struct AddStmt   { Expr *expr; std::string varName; };
struct RepeatStmt{ Expr *count; std::vector<Stmt *> body; };
struct IfStmt    { Expr *cond; std::vector<Stmt *> thenBody; std::vector<Stmt *> elseBody; };
struct WhileStmt { Expr *cond; std::vector<Stmt *> body; };
struct CallStmt  { std::string name; std::vector<Expr *> args; };
struct ProcedureStmt { std::string name; std::vector<std::string> params; std::vector<Stmt *> body; };

using StmtNode = std::variant<SayStmt, SetStmt, AddStmt, RepeatStmt, IfStmt, WhileStmt, CallStmt, ProcedureStmt>;
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
