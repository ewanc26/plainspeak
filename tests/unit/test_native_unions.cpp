#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents tagged unions", "[parser][unions][c99]") {
    Tokenizer tokenizer(
        "Union value: Field whole as integer. Field fraction as decimal. End union. "
        "Declare v as union value. Set member whole of v to 4. Say Member whole of v.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 4);
    const auto &uni = std::get<UnionStmt>(program[0]->node);
    CHECK(uni.name == "value");
    REQUIRE(uni.fields.size() == 2);
    CHECK(uni.fields[0].name == "whole");
    CHECK(std::holds_alternative<StoreMemberStmt>(program[2]->node));
}

TEST_CASE("sema completes unions and types members", "[sema][unions][c99]") {
    Tokenizer tokenizer(
        "Union node: Field value as integer. Field next as pointer to union node. End union. "
        "Declare n as union node. Set member value of n to 9. Say Member value of n.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());
    const auto &info = analysis.unions.at("node");
    REQUIRE(info.complete);
    REQUIRE(info.fields.size() == 2);
    CHECK(info.fields[0].second == Type::integer(IntegerRank::Int));
    CHECK(info.fields[1].second.isPointer());
    const auto &say = std::get<SayStmt>(program[3]->node);
    CHECK(analysis.exprTypes.at(say.expr) == Type::integer(IntegerRank::Int));
}

TEST_CASE("structure and union tags share the C tag namespace", "[sema][unions][diagnostics]") {
    Tokenizer tokenizer(
        "Structure value: Field x as integer. End structure. "
        "Union value: Field x as integer. End union.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 20);
}
