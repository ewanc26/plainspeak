#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

#include "../../src/ast/ast.h"
#include "../../src/lexer/tokenizer.h"
#include "../../src/parser/parser.h"
#include "../../src/sema/sema.h"

static std::vector<Diag> checkSource(const std::string &source) {
    Tokenizer tokenizer(source);
    Arena arena;
    Parser parser(tokenizer.tokenize(), arena);
    auto program = parser.parseProgram();
    Sema sema;
    return sema.check(program);
}

TEST_CASE("single trailing Return satisfies definite-return analysis", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f returns integer: Return 1. End procedure.").empty());
}

TEST_CASE("each path returning satisfies definite-return analysis", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "If n then: Return 1. End if. Return 2. End procedure.").empty());
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "If n then: Return 1. Else: Return 2. End if. End procedure.").empty());
}

TEST_CASE("a path that falls off the end without Return is rejected", "[sema][c99][control][error]") {
    auto diagnostics = checkSource("Procedure f takes n as integer returns integer: "
                                   "If n then: Return 1. End if. End procedure.");
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 18);
}

TEST_CASE("a non-void Procedure whose body is empty is rejected", "[sema][c99][control][error]") {
    auto diagnostics = checkSource("Procedure f returns integer: End procedure.");
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 18);
}

TEST_CASE("void Procedures need no Return at all", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f returns void: Say 1. End procedure.").empty());
    CHECK(checkSource("Procedure f returns void: End procedure.").empty());
}

TEST_CASE("a forever loop makes the function end unreachable", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f returns integer: While 1: Return 1. End while. End procedure.").empty());
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "Do: Return 1. End do while 1. End procedure.").empty());
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "Repeat 3: Return 1. End repeat. End procedure.").empty());
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "For i from 1 to 3: Return i. End for. End procedure.").empty());
}

TEST_CASE("a forever loop that can break still reaches the function end", "[sema][c99][control][error]") {
    auto diagnostics = checkSource("Procedure f takes n as integer returns integer: "
                                   "While 1: Break. End while. End procedure.");
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 18);
}

TEST_CASE("a Repeat body that can break still reaches the function end", "[sema][c99][control][error]") {
    auto diagnostics = checkSource("Procedure f takes n as integer returns integer: "
                                   "Repeat 3: Break. End repeat. End procedure.");
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 18);
}

TEST_CASE("a loop tail Return is honored when every path returns", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "While n: Return n. End while. Return 0. End procedure.").empty());
}

TEST_CASE("Switch with default and returning clauses satisfies definite-return analysis", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "Switch n: When 1: Return 1. Otherwise: Return 2. End switch. End procedure.").empty());
}

TEST_CASE("Switch without default falls through to the function end", "[sema][c99][control][error]") {
    auto diagnostics = checkSource("Procedure f takes n as integer returns integer: "
                                   "Switch n: When 1: Return 1. When 2: Return 2. End switch. End procedure.");
    REQUIRE(diagnostics.size() == 1);
    CHECK(diagnostics[0].code == 18);
}

TEST_CASE("goto-resolved returns satisfy definite-return analysis", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f takes n as integer returns integer: "
                      "If n then: Go to done. End if. Return 1. Label done. Return 0. End procedure.").empty());
}

TEST_CASE("dead code after a Return does not reintroduce a fall-off path", "[sema][c99][control]") {
    CHECK(checkSource("Procedure f returns integer: Return 1. Say 2. End procedure.").empty());
}