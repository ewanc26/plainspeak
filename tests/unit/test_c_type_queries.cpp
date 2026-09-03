#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

namespace {

Expr *parseSayExpr(const std::string &source, Arena &arena) {
    Tokenizer tokenizer(source);
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 1);
    return std::get<SayStmt>(program[0]->node).expr;
}

TypeSpecKind sizeKind(const std::string &typeText) {
    Arena arena;
    Expr *expr = parseSayExpr("Say Size of type " + typeText + ".", arena);
    return std::get<SizeOfTypeExpr>(expr->node).type.kind;
}

} // namespace

TEST_CASE("parser recognises ordinary C scalar type spellings", "[parser][c99]") {
    CHECK(sizeKind("boolean") == TypeSpecKind::Boolean);
    CHECK(sizeKind("character") == TypeSpecKind::Character);
    CHECK(sizeKind("signed character") == TypeSpecKind::SignedCharacter);
    CHECK(sizeKind("unsigned character") == TypeSpecKind::UnsignedCharacter);
    CHECK(sizeKind("short integer") == TypeSpecKind::ShortInteger);
    CHECK(sizeKind("unsigned short integer") == TypeSpecKind::UnsignedShortInteger);
    CHECK(sizeKind("integer") == TypeSpecKind::Integer);
    CHECK(sizeKind("unsigned integer") == TypeSpecKind::UnsignedInteger);
    CHECK(sizeKind("long integer") == TypeSpecKind::LongInteger);
    CHECK(sizeKind("unsigned long integer") == TypeSpecKind::UnsignedLongInteger);
    CHECK(sizeKind("long long integer") == TypeSpecKind::LongLongInteger);
    CHECK(sizeKind("unsigned long long integer") == TypeSpecKind::UnsignedLongLongInteger);
    CHECK(sizeKind("float") == TypeSpecKind::Float);
    CHECK(sizeKind("decimal") == TypeSpecKind::Decimal);
    CHECK(sizeKind("long decimal") == TypeSpecKind::LongDecimal);
}

TEST_CASE("alignment query has its own AST node", "[parser][c11]") {
    Arena arena;
    Expr *expr = parseSayExpr("Say Alignment of type unsigned long integer.", arena);
    const auto &alignment = std::get<AlignOfTypeExpr>(expr->node);
    CHECK(alignment.type.kind == TypeSpecKind::UnsignedLongInteger);
}

TEST_CASE("void type query is rejected by semantic analysis", "[sema][c99]") {
    Tokenizer tokenizer("Say Size of type void.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 12);
}
