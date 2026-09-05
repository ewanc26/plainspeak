#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents explicit conversion target type", "[parser][c99][cast]") {
    Tokenizer tokenizer("Say Convert 3.5 to type unsigned integer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &say = std::get<SayStmt>(program[0]->node);
    const auto *cast = std::get_if<CastExpr>(&say.args[0]->node);
    REQUIRE(cast != nullptr);
    CHECK(cast->target.kind == TypeSpecKind::UnsignedInteger);
    CHECK(std::holds_alternative<FloatLit>(cast->operand->node));
}

TEST_CASE("sema retains explicit arithmetic cast result type", "[sema][c99][cast]") {
    Tokenizer tokenizer("Say Convert 3.5 to type unsigned short integer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(analysis.exprTypes.at(say.args[0]) == Type::integer(IntegerRank::Short, true));
}

TEST_CASE("sema accepts object pointer and void pointer casts", "[sema][c99][cast][pointer]") {
    Tokenizer tokenizer(
        "Declare x as integer with value 9. "
        "Declare vp as pointer to void with value Convert Address of x to type pointer to void. "
        "Declare p as pointer to integer with value Convert vp to type pointer to integer. "
        "Say Value at p.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema accepts integer pointer casts and scalar to boolean", "[sema][c99][cast][pointer]") {
    Tokenizer tokenizer(
        "Declare p as pointer to integer with value Convert 0 to type pointer to integer. "
        "Declare x as integer. Say Convert Address of x to type boolean.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema rejects floating to pointer and aggregate casts", "[sema][c99][cast][error]") {
    Tokenizer tokenizer(
        "Structure box: Field x as integer. End structure. "
        "Declare b as structure box. "
        "Say Convert 1.5 to type pointer to integer. "
        "Say Convert b to type integer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 2);
    CHECK(diagnostics[0].code == 26);
    CHECK(diagnostics[1].code == 26);
}
