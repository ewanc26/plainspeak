#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents switch cases and default", "[parser][c99][switch]") {
    Tokenizer tokenizer(
        "Switch 2: Case 1: Say 1. Case 1 plus 1: Say 2. Default: Say 9. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &sw = std::get<SwitchStmt>(program[0]->node);
    REQUIRE(sw.cases.size() == 3);
    CHECK_FALSE(sw.cases[0].isDefault);
    CHECK_FALSE(sw.cases[1].isDefault);
    CHECK(sw.cases[2].isDefault);
}

TEST_CASE("switch accepts integer constant cases and break", "[sema][c99][switch]") {
    Tokenizer tokenizer(
        "Declare value as integer with value 2. "
        "Switch value: Case 1: Break. Case 1 plus 1: Break. Default: Break. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("continue in switch still needs an enclosing loop", "[sema][c99][switch][continue]") {
    Tokenizer tokenizer("Switch 1: Case 1: Continue. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 29);
}

TEST_CASE("switch rejects duplicate and nonconstant cases", "[sema][c99][switch][error]") {
    Tokenizer tokenizer(
        "Set runtime to 3. "
        "Switch 1: Case 1: Say 1. Case 1: Say 2. Case runtime: Say 3. End switch.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 32);
    CHECK(diagnostics[1].code == 32);
}
