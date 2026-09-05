#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents ascending and descending for loops", "[parser][c99][control]") {
    Tokenizer tokenizer("For i from 1 to 5: Say i. End for. For j from 5 down to 1: Say j. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 2);

    const auto &up = std::get<ForStmt>(program[0]->node);
    CHECK(up.varName == "i");
    CHECK(!up.descending);
    CHECK(std::holds_alternative<IntLit>(up.from->node));
    CHECK(std::holds_alternative<IntLit>(up.to->node));
    REQUIRE(up.body.size() == 1);
    CHECK(std::holds_alternative<SayStmt>(up.body[0]->node));

    const auto &down = std::get<ForStmt>(program[1]->node);
    CHECK(down.varName == "j");
    CHECK(down.descending);
    CHECK(std::holds_alternative<IntLit>(down.from->node));
    CHECK(std::holds_alternative<IntLit>(down.to->node));
    REQUIRE(down.body.size() == 1);
    CHECK(std::holds_alternative<SayStmt>(down.body[0]->node));
}

TEST_CASE("for each still parses beside the numeric for", "[parser][c99][control]") {
    Tokenizer tokenizer("For each x in xs: Say x. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<ForEachStmt>(program[0]->node));
}

TEST_CASE("for is a loop context for break and continue with a visible loop variable", "[sema][c99][control]") {
    Tokenizer tokenizer("For i from 1 to 5: Say i. Continue. Break. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("for loop variable is scoped to the body", "[sema][c99][control][error]") {
    Tokenizer tokenizer("For i from 1 to 3: Say i. End for. Say i.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 1);
}

TEST_CASE("for requires whole-number bound types", "[sema][c99][control][error]") {
    Tokenizer tokenizer("For i from 1.5 to 3: Say i. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 5);
}