#include <catch2/catch_test_macros.hpp>

#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents native object and recursive pointer prose", "[parser][native][pointer]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 41. "
        "Declare p as pointer to integer with value Address of x. "
        "Declare pp as pointer to pointer to integer with value Address of p. "
        "Set value at p to 42.");
    auto tokens = tokenizer.tokenize();
    Arena arena;
    Parser parser(tokens, arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 4);

    auto *x = std::get_if<NativeDeclStmt>(&program[0]->node);
    REQUIRE(x != nullptr);
    CHECK(x->type.kind == TypeSpecKind::Integer);
    REQUIRE(x->initializer != nullptr);

    auto *p = std::get_if<NativeDeclStmt>(&program[1]->node);
    REQUIRE(p != nullptr);
    CHECK(p->type.kind == TypeSpecKind::Pointer);
    REQUIRE(p->type.pointee);
    CHECK(p->type.pointee->kind == TypeSpecKind::Integer);

    auto *pp = std::get_if<NativeDeclStmt>(&program[2]->node);
    REQUIRE(pp != nullptr);
    CHECK(pp->type.kind == TypeSpecKind::Pointer);
    REQUIRE(pp->type.pointee);
    CHECK(pp->type.pointee->kind == TypeSpecKind::Pointer);
    REQUIRE(pp->type.pointee->pointee);
    CHECK(pp->type.pointee->pointee->kind == TypeSpecKind::Integer);

    CHECK(std::holds_alternative<StoreThroughStmt>(program[3]->node));
}

TEST_CASE("sema retains native declaration and pointer expression types", "[sema][native][pointer]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 41. "
        "Declare p as pointer to integer with value Address of x. "
        "Set value at p to 42. "
        "Say Value at p. "
        "Say Size of x.");
    auto tokens = tokenizer.tokenize();
    Arena arena;
    Parser parser(tokens, arena);
    auto program = parser.parseProgram();

    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());

    REQUIRE(analysis.declarationTypes.count(program[0]) == 1);
    CHECK(analysis.declarationTypes.at(program[0]) == Type::integer(IntegerRank::Int));

    REQUIRE(analysis.declarationTypes.count(program[1]) == 1);
    Type pointer = analysis.declarationTypes.at(program[1]);
    REQUIRE(pointer.isPointer());
    REQUIRE(pointer.elementType);
    CHECK(*pointer.elementType == Type::integer(IntegerRank::Int));

    auto *sayValue = std::get_if<SayStmt>(&program[3]->node);
    REQUIRE(sayValue != nullptr);
    REQUIRE(analysis.exprTypes.count(sayValue->args[0]) == 1);
    CHECK(analysis.exprTypes.at(sayValue->args[0]) == Type::integer(IntegerRank::Int));

    auto *saySize = std::get_if<SayStmt>(&program[4]->node);
    REQUIRE(saySize != nullptr);
    REQUIRE(analysis.typeOperands.count(saySize->args[0]) == 1);
    CHECK(analysis.typeOperands.at(saySize->args[0]) == Type::integer(IntegerRank::Int));
}

TEST_CASE("sema rejects addresses of boxed values", "[sema][pointer][error]") {
    Tokenizer tokenizer("Set x to 1. Declare p as pointer to long integer with value Address of x.");
    auto tokens = tokenizer.tokenize();
    Arena arena;
    Parser parser(tokens, arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 14);
}

TEST_CASE("sema rejects dereference of nonpointer values", "[sema][pointer][error]") {
    Tokenizer tokenizer("Say Value at 1.");
    auto tokens = tokenizer.tokenize();
    Arena arena;
    Parser parser(tokens, arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 15);
}
