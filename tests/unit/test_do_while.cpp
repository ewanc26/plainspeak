#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents do while loops", "[parser][c99][control]") {
    Tokenizer tokenizer("Do: Say 1. End do while 0.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &loop = std::get<DoWhileStmt>(program[0]->node);
    REQUIRE(loop.body.size() == 1);
    CHECK(std::holds_alternative<SayStmt>(loop.body[0]->node));
    CHECK(std::holds_alternative<IntLit>(loop.cond->node));
}

TEST_CASE("do while is a loop context for break and continue", "[sema][c99][control]") {
    Tokenizer tokenizer("Do: Continue. Break. End do while 1.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("do while requires a scalar condition", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Do: Say 1. End do while \"no\".");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 5);
}
