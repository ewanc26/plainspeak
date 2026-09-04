#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents native enumerations and qualified enumerators", "[parser][enums][c99]") {
    Tokenizer tokenizer(
        "Enumeration color: Enumerator red. Enumerator green as 5. Enumerator blue. End enumeration. "
        "Declare current as enumeration color with value Enumerator blue of color. "
        "Say Enumerator red of color.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 3);
    const auto &enumeration = std::get<EnumerationStmt>(program[0]->node);
    CHECK(enumeration.name == "color");
    REQUIRE(enumeration.enumerators.size() == 3);
    CHECK_FALSE(enumeration.enumerators[0].explicitValue);
    REQUIRE(enumeration.enumerators[1].explicitValue);
    CHECK(*enumeration.enumerators[1].explicitValue == 5);

    const auto &decl = std::get<NativeDeclStmt>(program[1]->node);
    CHECK(decl.type.kind == TypeSpecKind::Enumeration);
    REQUIRE(decl.initializer);
    CHECK(std::holds_alternative<EnumeratorExpr>(decl.initializer->node));
}

TEST_CASE("sema computes implicit and explicit enumerator values", "[sema][enums][c99]") {
    Tokenizer tokenizer(
        "Enumeration code: Enumerator below as minus 2. Enumerator next. Enumerator five as 5. Enumerator six. End enumeration. "
        "Declare value as enumeration code with value Enumerator six of code.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &info = analysis.enumerations.at("code");
    REQUIRE(info.complete);
    REQUIRE(info.enumerators.size() == 4);
    CHECK(info.enumerators[0].second == -2);
    CHECK(info.enumerators[1].second == -1);
    CHECK(info.enumerators[2].second == 5);
    CHECK(info.enumerators[3].second == 6);
}

TEST_CASE("sema rejects duplicate enumerator names", "[sema][enums][diagnostics]") {
    Tokenizer tokenizer(
        "Enumeration color: Enumerator red. Enumerator red as 4. End enumeration.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);

    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 22);
}
