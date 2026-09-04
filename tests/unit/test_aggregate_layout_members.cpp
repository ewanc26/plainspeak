#include <catch2/catch_test_macros.hpp>
#include "../../src/ast/ast.h"
#include "../../src/codegen/c_emitter.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

TEST_CASE("parser represents bit-fields and flexible array members", "[parser][aggregates][c99]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as unsigned integer. "
        "Bit field flags as unsigned integer with width 3. "
        "Bit field as unsigned integer with width 0. "
        "Flexible field data as unsigned character. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 1);
    const auto &structure = std::get<StructureStmt>(program[0]->node);
    REQUIRE(structure.fields.size() == 4);
    REQUIRE(structure.fields[1].bitWidth);
    CHECK(*structure.fields[1].bitWidth == 3);
    CHECK(structure.fields[2].name.empty());
    REQUIRE(structure.fields[2].bitWidth);
    CHECK(*structure.fields[2].bitWidth == 0);
    CHECK(structure.fields[3].flexibleArray);
}

TEST_CASE("sema records flexible structures and bit-field metadata", "[sema][aggregates][c99]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as unsigned integer. "
        "Bit field flags as unsigned integer with width 3. "
        "Flexible field data as unsigned character. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &info = analysis.structures.at("packet");
    CHECK(info.complete);
    CHECK(info.hasFlexibleArray);
    REQUIRE(info.fields.size() == 3);
    REQUIRE(info.fields[1].bitWidth);
    CHECK(*info.fields[1].bitWidth == 3);
    CHECK(info.fields[2].flexibleArray);
    CHECK(info.fields[2].type.isArray());
    CHECK_FALSE(info.fields[2].type.arrayBound);
}

TEST_CASE("sema rejects flexible structures embedded by value", "[sema][aggregates][diagnostics]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as integer. Flexible field data as character. End structure. "
        "Structure wrapper: Field packet as structure packet. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);

    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 23);
}


TEST_CASE("sema allows unions to contain flexible-array structures and propagates the restriction", "[sema][aggregates][c99]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as integer. Flexible field data as character. End structure. "
        "Union storage: Field packet as structure packet. Field marker as unsigned integer. End union.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    const auto &info = analysis.unions.at("storage");
    CHECK(info.complete);
    CHECK(info.hasFlexibleArray);
}

TEST_CASE("sema rejects a structure containing a flexible-array-bearing union", "[sema][aggregates][diagnostics]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as integer. Flexible field data as character. End structure. "
        "Union storage: Field packet as structure packet. Field marker as unsigned integer. End union. "
        "Structure wrapper: Field storage as union storage. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);

    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 23);
}

TEST_CASE("sema rejects fixed arrays of flexible-array-bearing unions", "[sema][aggregates][diagnostics]") {
    Tokenizer tokenizer(
        "Structure packet: Field length as integer. Flexible field data as character. End structure. "
        "Union storage: Field packet as structure packet. Field marker as unsigned integer. End union. "
        "Declare slots as array of union storage with length 2.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    auto diags = sema.check(program);

    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 17);
}

TEST_CASE("sema requires enumeration bit-field types to exist and be complete", "[sema][aggregates][diagnostics]") {
    {
        Tokenizer tokenizer("Structure flags: Bit field state as enumeration missing with width 1. End structure.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diags = sema.check(program);

        REQUIRE(diags.size() == 1);
        CHECK(diags[0].code == 23);
    }

    {
        Tokenizer tokenizer(
            "Structure flags: Bit field state as enumeration mode with width 1. End structure. "
            "Enumeration mode: Enumerator off. Enumerator on. End enumeration.");
        Arena arena;
        Parser parser(tokenizer.tokenize(), arena);
        auto program = parser.parseProgram();
        Sema sema;
        auto diags = sema.check(program);

        REQUIRE(diags.size() == 1);
        CHECK(diags[0].code == 23);
    }
}

TEST_CASE("codegen preserves native bit-field and flexible-array declarations", "[codegen][aggregates][c99]") {
    Tokenizer tokenizer(
        "Structure packet: "
        "Bit field low as unsigned integer with width 3. "
        "Bit field as unsigned integer with width 0. "
        "Flexible field data as unsigned character. End structure.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    AnalysisResult analysis = sema.analyze(program);

    REQUIRE(analysis.diagnostics.empty());
    std::string generated = emitProgram(program, analysis);

    CHECK(generated.find("unsigned int ps_low : 3;") != std::string::npos);
    CHECK(generated.find("unsigned int : 0;") != std::string::npos);
    CHECK(generated.find("unsigned char ps_data[];") != std::string::npos);
}
