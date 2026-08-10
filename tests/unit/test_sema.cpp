#include <catch2/catch_test_macros.hpp>
#include "../src/ast/ast.h"
#include "../src/sema/sema.h"

TEST_CASE("sema reports undeclared variable", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *say = arena.makeStmt(SayStmt{arena.makeExpr(VarRef{"y"}, 1)}, 1);
    program.push_back(say);

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 1);
}

TEST_CASE("sema accepts valid program", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *set = arena.makeStmt(SetStmt{"x", arena.makeExpr(IntLit{5}, 1)}, 1);
    auto *say = arena.makeStmt(SayStmt{arena.makeExpr(VarRef{"x"}, 1)}, 2);
    program.push_back(set);
    program.push_back(say);

    Sema sema;
    auto diags = sema.check(program);
    CHECK(diags.empty());
}

TEST_CASE("sema reports type mismatch in addition", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *set1 = arena.makeStmt(SetStmt{"x", arena.makeExpr(StringLit{"hello"}, 1)}, 1);
    auto *add = arena.makeStmt(AddStmt{arena.makeExpr(IntLit{5}, 2), "x"}, 2);
    program.push_back(set1);
    program.push_back(add);

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 3);
}

TEST_CASE("sema reports type mismatch in logical and", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *set1 = arena.makeStmt(SetStmt{"x", arena.makeExpr(StringLit{"hello"}, 1)}, 1);
    auto *set2 = arena.makeStmt(SetStmt{"y", arena.makeExpr(IntLit{5}, 2)}, 2);
    auto *andStmt = arena.makeStmt(SetStmt{"z", arena.makeExpr(BinaryExpr{BinOp::And, arena.makeExpr(VarRef{"x"}, 3), arena.makeExpr(VarRef{"y"}, 3)}, 3)}, 3);
    program.push_back(set1);
    program.push_back(set2);
    program.push_back(andStmt);

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 2);
}

TEST_CASE("sema accepts double literal", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *set = arena.makeStmt(SetStmt{"x", arena.makeExpr(FloatLit{3.14}, 1)}, 1);
    program.push_back(set);

    Sema sema;
    auto diags = sema.check(program);
    CHECK(diags.empty());
}

TEST_CASE("sema accepts mixed arithmetic", "[sema]") {
    std::vector<Stmt *> program;
    Arena arena;
    auto *set1 = arena.makeStmt(SetStmt{"x", arena.makeExpr(IntLit{5}, 1)}, 1);
    auto *set2 = arena.makeStmt(SetStmt{"y", arena.makeExpr(FloatLit{2.5}, 2)}, 2);
    auto *set3 = arena.makeStmt(SetStmt{"z", arena.makeExpr(BinaryExpr{BinOp::Add, arena.makeExpr(VarRef{"x"}, 3), arena.makeExpr(VarRef{"y"}, 3)}, 3)}, 3);
    program.push_back(set1);
    program.push_back(set2);
    program.push_back(set3);

    Sema sema;
    auto diags = sema.check(program);
    CHECK(diags.empty());
}
