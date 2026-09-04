#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents C23 null pointer value and type", "[parser][c23][nullptr]") {
    Tokenizer tokenizer("Declare n as null pointer type with value null pointer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 1);
    const auto &decl = std::get<NativeDeclStmt>(program[0]->node);
    CHECK(decl.type.kind == TypeSpecKind::Nullptr);
    REQUIRE(decl.initializer != nullptr);
    CHECK(std::holds_alternative<NullptrLit>(decl.initializer->node));
}

TEST_CASE("nullptr_t is distinct and converts to pointer and boolean", "[sema][c23][nullptr]") {
    Tokenizer tokenizer(
        "Declare n as null pointer type with value null pointer. "
        "Declare p as pointer to integer with value n. "
        "Declare b as boolean with value Convert n to type boolean.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());
    CHECK(analysis.declarationTypes.at(program[0]).kind == TypeKind::Nullptr);
}

TEST_CASE("literal zero is accepted as a pointer null constant", "[sema][c99][nullptr]") {
    Tokenizer tokenizer("Declare p as pointer to integer with value 0. Set p to 0.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("nonzero integer is not an implicit pointer conversion", "[sema][c99][nullptr]") {
    Tokenizer tokenizer("Declare p as pointer to integer with value 1.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 13);
}


TEST_CASE("literal zero converts to nullptr_t and compares equal", "[sema][c23][nullptr]") {
    Tokenizer tokenizer(
        "Declare n as null pointer type with value 0. "
        "Set n to 0. "
        "Declare m as null pointer type with value Convert 0 to type null pointer type. "
        "Say n is equal to 0. Say 0 is equal to m.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("nullptr_t conditional keeps nullptr_t result type", "[sema][c23][nullptr][conditional]") {
    Tokenizer tokenizer(
        "Declare flag as integer with value 1. "
        "Declare n as null pointer type with value "
        "Choose null pointer when flag otherwise null pointer.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());
    const auto &decl = std::get<NativeDeclStmt>(program[1]->node);
    REQUIRE(decl.initializer != nullptr);
    CHECK(analysis.exprTypes.at(decl.initializer).kind == TypeKind::Nullptr);
}

TEST_CASE("nullptr_t and integer zero are not conditional branch peers", "[sema][c23][nullptr][conditional]") {
    Tokenizer tokenizer(
        "Declare n as null pointer type with value "
        "Choose null pointer when 1 otherwise 0.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 28);
}

TEST_CASE("statement-form legacy calls reject nullptr_t arguments", "[sema][c23][nullptr][legacy]") {
    Tokenizer tokenizer(
        "Procedure sink takes value: Return value. End procedure. "
        "Call sink with null pointer done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 16);
}


TEST_CASE("nullptr_t relational comparisons are rejected", "[sema][c23][nullptr][comparison]") {
    Tokenizer tokenizer(
        "Declare left as null pointer type. Declare right as null pointer type. "
        "Say left is less than right.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diagnostics = sema.check(program);
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 4);
}
