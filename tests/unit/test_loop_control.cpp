#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents break and continue statements", "[parser][c99][control]") {
    Tokenizer tokenizer("While 1: Break. Continue. End while.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &loop = std::get<WhileStmt>(program[0]->node);
    REQUIRE(loop.body.size() == 2);
    CHECK(std::holds_alternative<BreakStmt>(loop.body[0]->node));
    CHECK(std::holds_alternative<ContinueStmt>(loop.body[1]->node));
}

TEST_CASE("sema accepts break and continue in all current loops", "[sema][c99][control]") {
    Tokenizer tokenizer(
        "Repeat 2: Continue. Break. End repeat. "
        "While 1: Continue. Break. End while. "
        "Set xs to List with 1 done. For each x in xs: Continue. Break. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema rejects break and continue outside a loop", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Break. Continue.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 29);
    CHECK(diagnostics[1].code == 29);
}

TEST_CASE("loop context is restored after nested loop in procedure", "[sema][control][procedure]") {
    Tokenizer tokenizer(
        "Procedure inner: While 1: Break. End while. Break. Return 0. End procedure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 29);
}
