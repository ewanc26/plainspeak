#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/codegen/c_emitter.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents static assertions with optional messages", "[parser][c11][static-assert]") {
    Tokenizer tokenizer(
        "Assert that 1. "
        "Assert that 2 plus 2 is equal to 4 with message \"math still works\".");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 2);
    const auto &first = std::get<StaticAssertStmt>(program[0]->node);
    CHECK_FALSE(first.message.has_value());
    const auto &second = std::get<StaticAssertStmt>(program[1]->node);
    REQUIRE(second.message.has_value());
    CHECK(*second.message == "math still works");
}

TEST_CASE("sema accepts true integer constant static assertions", "[sema][c11][static-assert]") {
    Tokenizer tokenizer(
        "Assert that 2 plus 2 is equal to 4. "
        "Assert that true or 1 divided by 0.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema diagnoses failed static assertion with message", "[sema][c11][static-assert][error]") {
    Tokenizer tokenizer("Assert that 1 is equal to 2 with message \"numbers disagree\".");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 37);
    CHECK(diagnostics[0].message == "Static assertion failed: numbers disagree.");
}

TEST_CASE("sema rejects runtime static assertion expressions", "[sema][c11][static-assert][error]") {
    Tokenizer tokenizer("Set value to 1. Assert that value.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);

    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 37);
}

TEST_CASE("static assertion message is retained in native C lowering", "[codegen][c11][static-assert]") {
    Tokenizer tokenizer("Assert that 4 is greater than 0 with message \"positive\".");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    CHECK(emitProgram(program, analysis).find("_Static_assert(") != std::string::npos);
    CHECK(emitProgram(program, analysis).find("\"positive\");") != std::string::npos);
}
