#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents choose conditional expression", "[parser][c99][conditional]") {
    Tokenizer tokenizer("Say Choose 10 when 1 otherwise 20.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &say = std::get<SayStmt>(program[0]->node);
    const auto *choose = std::get_if<ConditionalExpr>(&say.args[0]->node);
    REQUIRE(choose != nullptr);
    CHECK(std::holds_alternative<IntLit>(choose->whenTrue->node));
    CHECK(std::holds_alternative<IntLit>(choose->condition->node));
    CHECK(std::holds_alternative<IntLit>(choose->whenFalse->node));
}

TEST_CASE("conditional arithmetic uses usual arithmetic conversions", "[sema][c99][conditional]") {
    Tokenizer tokenizer(
        "Declare a as unsigned integer with value 1. "
        "Declare b as long integer with value 2. "
        "Say Choose a when 1 otherwise b.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &say = std::get<SayStmt>(program[2]->node);
    CHECK(analysis.exprTypes.at(say.args[0]) == Type::integer(IntegerRank::Long));
}

TEST_CASE("conditional pointer result composes pointee qualifiers", "[sema][c99][conditional][pointer]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 1. "
        "Declare p as pointer to integer with value Address of x. "
        "Declare cp as pointer to constant integer with value p. "
        "Declare r as pointer to constant integer with value Choose p when 1 otherwise cp.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &decl = std::get<NativeDeclStmt>(program[3]->node);
    REQUIRE(decl.initializer != nullptr);
    Type result = analysis.exprTypes.at(decl.initializer);
    REQUIRE(result.isPointer());
    REQUIRE(result.elementType);
    CHECK(result.elementType->kind == TypeKind::Integer);
    CHECK(result.elementType->qualifiers.isConst);
}

TEST_CASE("conditional accepts object pointer and void pointer branches", "[sema][c99][conditional][pointer]") {
    Tokenizer tokenizer(
        "Declare x as integer. "
        "Declare p as pointer to integer with value Address of x. "
        "Declare v as pointer to void with value Convert p to type pointer to void. "
        "Declare r as pointer to void with value Choose p when 1 otherwise v.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("conditional rejects non scalar condition and incompatible branches", "[sema][c99][conditional][error]") {
    Tokenizer tokenizer(
        "Say Choose 1 when \"no\" otherwise 2. "
        "Declare x as integer. Declare y as decimal. "
        "Declare p as pointer to integer with value Address of x. "
        "Declare q as pointer to decimal with value Address of y. "
        "Say Choose p when 1 otherwise q.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 28);
    CHECK(diagnostics[1].code == 28);
}
