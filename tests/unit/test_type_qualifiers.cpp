#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser preserves qualifier placement recursively", "[parser][qualifiers][c11]") {
    Tokenizer tokenizer(
        "Declare a as constant pointer to integer. "
        "Declare b as pointer to constant integer. "
        "Declare c as volatile restricted pointer to atomic integer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 3);
    const auto &a = std::get<NativeDeclStmt>(program[0]->node);
    const auto &b = std::get<NativeDeclStmt>(program[1]->node);
    const auto &c = std::get<NativeDeclStmt>(program[2]->node);

    CHECK(a.type.qualifiers.isConst);
    REQUIRE(a.type.pointee);
    CHECK_FALSE(a.type.pointee->qualifiers.isConst);

    CHECK_FALSE(b.type.qualifiers.isConst);
    REQUIRE(b.type.pointee);
    CHECK(b.type.pointee->qualifiers.isConst);

    CHECK(c.type.qualifiers.isVolatile);
    CHECK(c.type.qualifiers.isRestrict);
    REQUIRE(c.type.pointee);
    CHECK(c.type.pointee->qualifiers.isAtomic);
}

TEST_CASE("sema allows adding but not discarding pointee qualifiers", "[sema][qualifiers][c11]") {
    {
        Tokenizer tokenizer(
            "Declare x as integer. Declare p as pointer to integer with value Address of x. "
            "Declare cp as pointer to constant integer with value p.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        CHECK(sema.check(program).empty());
    }
    {
        Tokenizer tokenizer(
            "Declare x as integer. Declare cp as pointer to constant integer with value Address of x. "
            "Declare p as pointer to integer with value cp.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diags = sema.check(program);
        REQUIRE(diags.size() == 1);
        CHECK(diags[0].code == 13);
    }
}

TEST_CASE("sema rejects invalid restrict and atomic array qualification", "[sema][qualifiers][c11]") {
    Tokenizer tokenizer(
        "Declare x as restricted integer. "
        "Declare a as atomic array of integer with length 2.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);

    REQUIRE(diags.size() == 2);
    CHECK(diags[0].code == 24);
    CHECK(diags[1].code == 24);
}
