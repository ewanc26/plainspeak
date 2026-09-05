#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents goto and label statements", "[parser][c99][control]") {
    Tokenizer tokenizer("Go to top. Label top. Say \"here\".");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 3);
    const auto &jump = std::get<GotoStmt>(program[0]->node);
    CHECK(jump.label == "top");
    const auto &mark = std::get<LabelStmt>(program[1]->node);
    CHECK(mark.name == "top");
    CHECK(std::holds_alternative<SayStmt>(program[2]->node));
}

TEST_CASE("forward and backward gotos are accepted in one function", "[sema][c99][control]") {
    Tokenizer tokenizer("Set n to 0. Go to end. Label again. Add 1 to n. If n is less than 3 then: Go to again. End if. Label end.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("goto into a switch body is accepted; break context still applies inside the switch", "[sema][c99][control]") {
    Tokenizer tokenizer("Go to duel. Set x to 1. Switch x: When 1: Say \"one\". Label duel. When 2: Break. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("goto to an undefined label is rejected", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Go to nowhere.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 32);
}

TEST_CASE("duplicate labels are rejected", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Label same. Label same.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 32);
}

TEST_CASE("label namespaces are function-scoped", "[sema][c99][control][error]") {
    {
        // A label inside a procedure does not satisfy a top-level goto.
        Tokenizer tokenizer("Procedure p: Label secret. End procedure. Go to secret.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diagnostics = sema.check(program);
        REQUIRE(diagnostics.size() == 1);
        CHECK(diagnostics[0].code == 32);
    }
    {
        // The same label name may exist in both scopes without colliding,
        // and each goto resolves within its own function.
        Tokenizer tokenizer("Procedure p: Go to same. Label same. End procedure. Go to same. Label same.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        CHECK(sema.check(program).empty());
    }
}

TEST_CASE("goto cannot bridge a procedure boundary even when the label exists elsewhere", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Label outer. Procedure p: Go to outer. End procedure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 32);
}