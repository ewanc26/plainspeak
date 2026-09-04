#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents native structure fields and member access", "[parser][structures][c99]") {
    Tokenizer tokenizer(
        "Structure point: Field x as integer. Field y as integer. End structure. "
        "Declare p as structure point. Set member x of p to 3. Say Member x of p.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 4);
    const auto &structure = std::get<StructureStmt>(program[0]->node);
    CHECK(structure.name == "point");
    REQUIRE(structure.fields.size() == 2);
    CHECK(structure.fields[0].name == "x");
    CHECK(structure.fields[0].type.kind == TypeSpecKind::Integer);
    CHECK(std::holds_alternative<StoreMemberStmt>(program[2]->node));
    const auto &say = std::get<SayStmt>(program[3]->node);
    CHECK(std::holds_alternative<MemberExpr>(say.expr->node));
}

TEST_CASE("sema completes structures and types members", "[sema][structures][c99]") {
    Tokenizer tokenizer(
        "Structure node: Field value as integer. Field next as pointer to structure node. End structure. "
        "Declare n as structure node. Set member value of n to 9. Say Member value of n.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());
    const auto &info = analysis.structures.at("node");
    REQUIRE(info.complete);
    REQUIRE(info.fields.size() == 2);
    CHECK(info.fields[0].second == Type::integer(IntegerRank::Int));
    CHECK(info.fields[1].second.isPointer());
    const auto &say = std::get<SayStmt>(program[3]->node);
    CHECK(analysis.exprTypes.at(say.expr) == Type::integer(IntegerRank::Int));
}

TEST_CASE("sema rejects recursive structure fields by value", "[sema][structures][diagnostics]") {
    Tokenizer tokenizer(
        "Structure node: Field next as structure node. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 19);
}
