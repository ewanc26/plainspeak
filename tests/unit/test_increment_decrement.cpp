#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser distinguishes prefix and postfix increment decrement", "[parser][c99][incdec]") {
    Tokenizer tokenizer(
        "Declare x as integer. "
        "Say Increment before x. Say Increment after x. "
        "Say Decrement before x. Say Decrement after x.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 5);
    CHECK(std::get<IncDecExpr>(std::get<SayStmt>(program[1]->node).expr->node).kind == IncDecKind::PrefixIncrement);
    CHECK(std::get<IncDecExpr>(std::get<SayStmt>(program[2]->node).expr->node).kind == IncDecKind::PostfixIncrement);
    CHECK(std::get<IncDecExpr>(std::get<SayStmt>(program[3]->node).expr->node).kind == IncDecKind::PrefixDecrement);
    CHECK(std::get<IncDecExpr>(std::get<SayStmt>(program[4]->node).expr->node).kind == IncDecKind::PostfixDecrement);
}

TEST_CASE("increment result drops top level qualifiers", "[sema][c99][incdec][qualifiers]") {
    Tokenizer tokenizer("Declare x as volatile integer with value 1. Say Increment after x.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &say = std::get<SayStmt>(program[1]->node);
    CHECK(analysis.exprTypes.at(say.expr) == Type::integer(IntegerRank::Int));
}

TEST_CASE("increment accepts atomic real objects", "[sema][c11][incdec][atomic]") {
    Tokenizer tokenizer("Declare x as atomic integer with value 1. Say Increment after x.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("increment rejects boxed and temporary operands", "[sema][c99][incdec][error]") {
    Tokenizer tokenizer("Set x to 1. Say Increment before x. Say Increment after (1 plus 2).");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 27);
    CHECK(diagnostics[1].code == 27);
}

TEST_CASE("pointer increment requires complete object pointee", "[sema][c99][incdec][pointer]") {
    Tokenizer tokenizer(
        "Declare p as pointer to structure node. "
        "Say Increment before p. "
        "Structure node: Field value as integer. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 27);
}
