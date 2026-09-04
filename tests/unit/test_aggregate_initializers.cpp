#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents positional and designated aggregate initializers", "[parser][initializers][c99]") {
    Tokenizer tokenizer(
        "Structure point: Field x as integer. Field y as integer. End structure. "
        "Declare p as structure point with members y as 4 followed by x as 3 done. "
        "Declare a as array of integer with length 4 with elements at 2 as 9 followed by at 0 as 1 done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 3);

    const auto &p = std::get<NativeDeclStmt>(program[1]->node);
    REQUIRE(p.aggregateInitializer);
    CHECK(p.aggregateInitializer->kind == AggregateInitKind::Members);
    REQUIRE(p.aggregateInitializer->entries.size() == 2);
    CHECK(p.aggregateInitializer->entries[0].memberName == "y");

    const auto &a = std::get<NativeDeclStmt>(program[2]->node);
    REQUIRE(a.aggregateInitializer);
    CHECK(a.aggregateInitializer->kind == AggregateInitKind::Elements);
    REQUIRE(a.aggregateInitializer->entries.size() == 2);
    CHECK(a.aggregateInitializer->entries[0].elementIndex == 2);
}

TEST_CASE("sema accepts valid positional and designated aggregate initializers", "[sema][initializers][c99]") {
    Tokenizer tokenizer(
        "Structure point: Field x as integer. Field y as integer. End structure. "
        "Union value: Field whole as integer. Field fraction as decimal. End union. "
        "Declare p as structure point with values 3 followed by 4 done. "
        "Declare q as structure point with members y as 8 done. "
        "Declare a as array of integer with length 4 with elements at 3 as 7 followed by at 1 as 2 done. "
        "Declare u as union value with members fraction as 2.5 done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema rejects duplicate designated aggregate members", "[sema][initializers][diagnostics]") {
    Tokenizer tokenizer(
        "Structure point: Field x as integer. End structure. "
        "Declare p as structure point with members x as 1 followed by x as 2 done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 21);
}
