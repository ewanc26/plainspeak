#include <catch2/catch_test_macros.hpp>
#include "../src/ast/ast.h"
#include "../src/lexer/tokenizer.h"
#include "../src/parser/parser.h"

TEST_CASE("parser parses Say statement", "[parser]") {
    Tokenizer t("Say \"hello\".");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<SayStmt>(program[0]->node));
}

TEST_CASE("parser parses Set statement", "[parser]") {
    Tokenizer t("Set x to 5.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &s = std::get<SetStmt>(program[0]->node);
    CHECK(s.name == "x");
}

TEST_CASE("parser parses If with else", "[parser]") {
    Tokenizer t("If x is greater than 5 then:\n    Say \"big\".\nElse:\n    Say \"small\".\nEnd if.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &ifstmt = std::get<IfStmt>(program[0]->node);
    CHECK(ifstmt.thenBody.size() == 1);
    CHECK(ifstmt.elseBody.size() == 1);
}

TEST_CASE("parser parses While loop", "[parser]") {
    Tokenizer t("While x is less than 3:\n    Say x.\nEnd while.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<WhileStmt>(program[0]->node));
}

TEST_CASE("parser parses Procedure and Call", "[parser]") {
    Tokenizer t("Procedure greet takes name:\n    Say name.\nEnd procedure.\nCall greet with \"world\".");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<ProcedureStmt>(program[0]->node));
    CHECK(std::holds_alternative<CallStmt>(program[1]->node));
}

TEST_CASE("parser handles arithmetic precedence", "[parser]") {
    Tokenizer t("Say 2 plus 3 times 4.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    auto &expr = std::get<BinaryExpr>(say.expr->node);
    CHECK(expr.op == BinOp::Add);
    CHECK(std::holds_alternative<BinaryExpr>(expr.rhs->node));
    auto &mul = std::get<BinaryExpr>(expr.rhs->node);
    CHECK(mul.op == BinOp::Mul);
}

TEST_CASE("parser handles logical operators", "[parser]") {
    Tokenizer t("Say true and false or true.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    auto &orExpr = std::get<BinaryExpr>(say.expr->node);
    CHECK(orExpr.op == BinOp::Or);
    CHECK(std::holds_alternative<BinaryExpr>(orExpr.lhs->node));
    auto &andExpr = std::get<BinaryExpr>(orExpr.lhs->node);
    CHECK(andExpr.op == BinOp::And);
}

TEST_CASE("parser handles not operator", "[parser]") {
    Tokenizer t("Say not true.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<UnaryExpr>(say.expr->node));
    auto &notExpr = std::get<UnaryExpr>(say.expr->node);
    CHECK(notExpr.op == UnaryOp::Not);
}
