#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("integer constant expressions form null pointer constants", "[sema][c99][constant-expression][nullptr]") {
    Tokenizer tokenizer(
        "Declare p as pointer to integer with value 1 minus 1. "
        "Declare q as pointer to integer with value 2 times 0. "
        "Declare n as null pointer type with value false. "
        "Say p is equal to (3 minus 3).");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("runtime integer zero is not a null pointer constant", "[sema][c99][constant-expression][nullptr][error]") {
    Tokenizer tokenizer(
        "Set zero to 0. "
        "Declare p as pointer to integer with value zero.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 13);
}

TEST_CASE("short-circuit integer constant expressions preserve unevaluated branches", "[sema][c99][constant-expression][nullptr]") {
    Tokenizer tokenizer(
        "Procedure zero returns integer: Return 0. End procedure. "
        "Declare p as pointer to integer with value false and Call zero done. "
        "Declare q as pointer to integer with value (1 minus 1) times Call zero done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 13);
}

TEST_CASE("constant zero participates in pointer conditional expressions", "[sema][c99][constant-expression][conditional]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 7. "
        "Declare flag as integer with value 1. "
        "Declare p as pointer to integer with value "
        "Choose Address of x when flag otherwise 4 minus 4.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}
