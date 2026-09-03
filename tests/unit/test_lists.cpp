#include <catch2/catch_test_macros.hpp>
#include "../src/ast/ast.h"
#include "../src/lexer/tokenizer.h"
#include "../src/parser/parser.h"
#include "../src/sema/sema.h"

TEST_CASE("parser accepts a complete list program as one paragraph", "[parser][lists]") {
    Tokenizer tokenizer("Set primes to List with 2 followed by 3 followed by 5 done. (Mutate it in prose.) Append 7 to primes. Replace item at 2 in primes with 11. Remove item at 1 from primes. For each prime in primes: Say prime. End for.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    REQUIRE(program.size() == 6);
    CHECK(std::holds_alternative<SetStmt>(program[0]->node));
    CHECK(std::holds_alternative<CommentStmt>(program[1]->node));
    CHECK(std::holds_alternative<AppendStmt>(program[2]->node));
    CHECK(std::holds_alternative<ReplaceItemStmt>(program[3]->node));
    CHECK(std::holds_alternative<RemoveItemStmt>(program[4]->node));
    CHECK(std::holds_alternative<ForEachStmt>(program[5]->node));
}

TEST_CASE("sema accepts homogeneous list operations", "[sema][lists]") {
    Tokenizer tokenizer("Set names to Empty list of strings. Append \"Ada\" to names. Append \"Grace\" to names. Say Item at 2 in names. Say Length of names.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    CHECK(sema.check(program).empty());
}

TEST_CASE("sema rejects mixed list literals", "[sema][lists]") {
    Tokenizer tokenizer("Set values to List with 1 followed by \"two\" done.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 9);
}

TEST_CASE("sema rejects non-integral list positions", "[sema][lists]") {
    Tokenizer tokenizer("Set values to List with 1 followed by 2 done. Say Item at 1.5 in values.");
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();

    Sema sema;
    auto diags = sema.check(program);
    REQUIRE(diags.size() == 1);
    CHECK(diags[0].code == 11);
}
