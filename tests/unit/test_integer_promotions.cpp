#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

namespace {
AnalysisResult analyze(const std::string &source, Arena &arena) {
    Tokenizer tokenizer(source);
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    return sema.analyze(program);
}
}

TEST_CASE("integer promotions lift small native integers to int", "[sema][c99][promotions]") {
    Tokenizer tokenizer(
        "Declare a as unsigned character with value 7. "
        "Declare b as short integer with value 2. "
        "Say a plus b. Say bitwise not a. Say a shifted left by b.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());

    const auto &sum = std::get<SayStmt>(program[2]->node);
    const auto &complement = std::get<SayStmt>(program[3]->node);
    const auto &shift = std::get<SayStmt>(program[4]->node);
    CHECK(analysis.exprTypes.at(sum.expr) == Type::integer(IntegerRank::Int));
    CHECK(analysis.exprTypes.at(complement.expr) == Type::integer(IntegerRank::Int));
    CHECK(analysis.exprTypes.at(shift.expr) == Type::integer(IntegerRank::Int));
}

TEST_CASE("usual integer conversions preserve unsigned rank rules", "[sema][c99][conversions]") {
    Tokenizer tokenizer(
        "Declare a as integer with value 1. "
        "Declare b as unsigned integer with value 2. "
        "Declare c as long integer with value 3. "
        "Declare d as unsigned long integer with value 4. "
        "Say a plus b. Say c bitwise xor d.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());

    const auto &intMixed = std::get<SayStmt>(program[4]->node);
    const auto &longMixed = std::get<SayStmt>(program[5]->node);
    CHECK(analysis.exprTypes.at(intMixed.expr) == Type::integer(IntegerRank::Int, true));
    CHECK(analysis.exprTypes.at(longMixed.expr) == Type::integer(IntegerRank::Long, true));
}

TEST_CASE("usual arithmetic conversions retain floating rank", "[sema][c99][conversions]") {
    Tokenizer tokenizer(
        "Declare f as float with value 1. "
        "Declare d as decimal with value 2. "
        "Declare ld as long decimal with value 3. "
        "Say f plus d. Say d times ld.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());

    const auto &doubleExpr = std::get<SayStmt>(program[3]->node);
    const auto &longDoubleExpr = std::get<SayStmt>(program[4]->node);
    CHECK(analysis.exprTypes.at(doubleExpr.expr) == Type::floating(FloatingRank::Double));
    CHECK(analysis.exprTypes.at(longDoubleExpr.expr) == Type::floating(FloatingRank::LongDouble));
}

TEST_CASE("bitwise and modulo reject floating operands", "[sema][c99][bitwise]") {
    Tokenizer tokenizer("Say 1.5 bitwise and 1. Say 3.0 mod 2.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 25);
    CHECK(diagnostics[1].code == 2);
}
