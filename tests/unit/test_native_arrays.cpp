#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"

TEST_CASE("parser recognises recursive fixed native array types", "[parser][arrays][c99]") {
    Tokenizer tokenizer("Declare values as array of pointer to integer with length 4.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 1);
    const auto &decl = std::get<NativeDeclStmt>(program[0]->node);
    CHECK(decl.type.kind == TypeSpecKind::Array);
    CHECK(decl.type.arrayBound == 4);
    REQUIRE(decl.type.pointee);
    CHECK(decl.type.pointee->kind == TypeSpecKind::Pointer);
    REQUIRE(decl.type.pointee->pointee);
    CHECK(decl.type.pointee->pointee->kind == TypeSpecKind::Integer);
}

TEST_CASE("parser keeps native element syntax separate from list item syntax", "[parser][arrays][c99]") {
    Tokenizer tokenizer("Declare values as array of integer with length 2. Set element at 0 in values to 7. Say Element at 0 in values.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 3);
    CHECK(std::holds_alternative<StoreElementStmt>(program[1]->node));
    const auto &say = std::get<SayStmt>(program[2]->node);
    CHECK(std::holds_alternative<ElementExpr>(say.args[0]->node));
}

#include "../../src/sema/sema.h"

TEST_CASE("sema resolves fixed arrays, decay and pointer arithmetic", "[sema][arrays][pointer][c99]") {
    Tokenizer tokenizer(
        "Declare values as array of integer with length 4. "
        "Declare p as pointer to integer with value values. "
        "Declare q as pointer to integer with value p plus 2. "
        "Say Element at 1 in q. "
        "Say q minus p.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());

    Type array = analysis.declarationTypes.at(program[0]);
    REQUIRE(array.isArray());
    REQUIRE(array.arrayBound);
    CHECK(*array.arrayBound == 4);
    REQUIRE(array.elementType);
    CHECK(*array.elementType == Type::integer(IntegerRank::Int));

    Type p = analysis.declarationTypes.at(program[1]);
    REQUIRE(p.isPointer());
    REQUIRE(p.elementType);
    CHECK(*p.elementType == Type::integer(IntegerRank::Int));

    const auto &qDecl = std::get<NativeDeclStmt>(program[2]->node);
    REQUIRE(qDecl.initializer);
    CHECK(analysis.exprTypes.at(qDecl.initializer).isPointer());

    const auto &sayElement = std::get<SayStmt>(program[3]->node);
    CHECK(analysis.exprTypes.at(sayElement.args[0]) == Type::integer(IntegerRank::Int));
    const auto &sayDifference = std::get<SayStmt>(program[4]->node);
    CHECK(analysis.exprTypes.at(sayDifference.args[0]) == Type::number());
}


TEST_CASE("invalid pointer initializer reports the root expression error once", "[sema][pointer][diagnostics]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 1. "
        "Declare p as pointer to integer with value Address of x. "
        "Declare q as pointer to integer with value p plus p.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 16);
}
