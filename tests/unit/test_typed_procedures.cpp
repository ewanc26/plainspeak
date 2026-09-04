#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents typed procedure parameters and return type", "[parser][procedures][types]") {
    Tokenizer tokenizer(
        "Procedure move takes where as pointer to integer amount as integer returns integer: "
        "Return Value at where plus amount. End procedure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    REQUIRE(program.size() == 1);
    const auto &proc = std::get<ProcedureStmt>(program[0]->node);
    REQUIRE(proc.params.size() == 2);
    CHECK(proc.params[0].name == "where");
    REQUIRE(proc.params[0].type);
    CHECK(proc.params[0].type->kind == TypeSpecKind::Pointer);
    REQUIRE(proc.params[1].type);
    CHECK(proc.params[1].type->kind == TypeSpecKind::Integer);
    REQUIRE(proc.returnType);
    CHECK(proc.returnType->kind == TypeSpecKind::Integer);
}

TEST_CASE("typed array parameters undergo C parameter adjustment", "[sema][procedures][arrays]") {
    Tokenizer tokenizer(
        "Procedure first takes values as array of integer with length 3 returns integer: "
        "Return Element at 0 in values. End procedure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    REQUIRE(analysis.diagnostics.empty());
    const auto &sig = analysis.procedureSignatures.at("first");
    REQUIRE(sig.nativeTyped);
    REQUIRE(sig.parameterTypes.size() == 1);
    CHECK(sig.parameterTypes[0].isPointer());
    REQUIRE(sig.parameterTypes[0].elementType);
    CHECK(*sig.parameterTypes[0].elementType == Type::integer(IntegerRank::Int));
    CHECK(sig.returnType == Type::integer(IntegerRank::Int));
}

TEST_CASE("typed forward and mutual procedure calls resolve from preregistered signatures", "[sema][procedures][recursion]") {
    Tokenizer tokenizer(
        "Procedure even takes n as integer returns integer: "
        "If n is equal to 0 then: Return 1. End if. "
        "Return Call odd with n minus 1 done. End procedure. "
        "Procedure odd takes n as integer returns integer: "
        "If n is equal to 0 then: Return 0. End if. "
        "Return Call even with n minus 1 done. End procedure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("typed void procedures accept bare return", "[sema][procedures][void]") {
    Tokenizer tokenizer("Procedure noop returns void: Return. End procedure. Call noop done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    CHECK(sema.check(program).empty());
}
