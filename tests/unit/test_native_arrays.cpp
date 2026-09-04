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
    CHECK(std::holds_alternative<ElementExpr>(say.expr->node));
}
