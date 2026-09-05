#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents switch with when and otherwise clauses", "[parser][c99][control]") {
    Tokenizer tokenizer("Switch 7: When 1: Say \"one\". Break. When 7: Say \"seven\". Break. "
                        "Otherwise: Say \"other\". Break. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &s = std::get<SwitchStmt>(program[0]->node);
    CHECK(std::holds_alternative<IntLit>(s.cond->node));
    REQUIRE(s.cases.size() == 3);

    const auto &first = s.cases[0];
    REQUIRE(first.value != nullptr);
    CHECK(std::holds_alternative<IntLit>(first.value->node));
    REQUIRE(first.body.size() == 2);
    CHECK(std::holds_alternative<SayStmt>(first.body[0]->node));
    CHECK(std::holds_alternative<BreakStmt>(first.body[1]->node));

    const auto &def = s.cases[2];
    CHECK(def.value == nullptr);
    REQUIRE(def.body.size() == 2);
}

TEST_CASE("parser accepts fall-through when labels with empty bodies", "[parser][c99][control]") {
    Tokenizer tokenizer("Switch 4: When 4: When 5: Say \"five\". Break. Otherwise: Say \"other\". End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &s = std::get<SwitchStmt>(program[0]->node);
    REQUIRE(s.cases.size() == 3);
    CHECK(s.cases[0].body.empty());
    CHECK(s.cases[1].body.size() == 2);
}

TEST_CASE("switch is a breakable context and accepts break but not loop-only continue", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Switch 1: When 1: Break. When 2: Continue. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 29);
}

TEST_CASE("continue inside a switch within a loop continues the loop", "[sema][c99][control]") {
    Tokenizer tokenizer("For i from 1 to 3: Switch i: When 2: Continue. End switch. Break. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("switch requires a whole-number condition", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Switch 1.5: When 1: Say \"one\". End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 30);
}

TEST_CASE("when labels must be integer constants", "[sema][c99][control][error]") {
    Tokenizer tokenizer("Set v to 3. Switch 1: When v: Say \"x\". End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 30);
}

TEST_CASE("duplicate when values and duplicate otherwise clauses are rejected", "[sema][c99][control][error]") {
    {
        Tokenizer tokenizer("Switch 1: When 1: Say \"a\". When 1: Say \"b\". End switch.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diagnostics = sema.check(program);
        REQUIRE(diagnostics.size() == 1);
        CHECK(diagnostics[0].code == 30);
    }
    {
        Tokenizer tokenizer("Switch 1: Otherwise: Say \"a\". Otherwise: Say \"b\". End switch.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diagnostics = sema.check(program);
        REQUIRE(diagnostics.size() == 1);
        CHECK(diagnostics[0].code == 30);
    }
}

TEST_CASE("validated switch labels are recorded for codegen", "[sema][c99][control]") {
    Tokenizer tokenizer("Switch 2 plus 3: When 5: Say \"five\". End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto analysis = sema.analyze(program);

    CHECK(analysis.diagnostics.empty());
    const auto &s = std::get<SwitchStmt>(program[0]->node);
    REQUIRE(analysis.switchCaseValues.size() == 1);
    CHECK(analysis.switchCaseValues.at(s.cases[0].value) == 5);
}

TEST_CASE("parser rejects an empty switch block", "[parser][error]") {
    Tokenizer tokenizer("Switch 1: End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    CHECK_THROWS_AS(parser.parseProgram(), std::exception);
}