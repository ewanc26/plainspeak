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
    Tokenizer t("Procedure greet takes name:\n    Say name.\nEnd procedure.\nCall greet with \"world\" done.");
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
    auto &expr = std::get<BinaryExpr>(say.args[0]->node);
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
    auto &orExpr = std::get<BinaryExpr>(say.args[0]->node);
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
    CHECK(std::holds_alternative<UnaryExpr>(say.args[0]->node));
    auto &notExpr = std::get<UnaryExpr>(say.args[0]->node);
    CHECK(notExpr.op == UnaryOp::Not);
}

TEST_CASE("parser parses Subtract statement", "[parser]") {
    Tokenizer t("Subtract 1 from total.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<SubStmt>(program[0]->node));
}

TEST_CASE("parser parses Return statement", "[parser]") {
    Tokenizer t("Return x plus 1.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<ReturnStmt>(program[0]->node));
}

TEST_CASE("parser parses parenthetical Comment statement", "[parser]") {
    Tokenizer t("(This is a comment.)");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    CHECK(std::holds_alternative<CommentStmt>(program[0]->node));
    auto &cs = std::get<CommentStmt>(program[0]->node);
    CHECK(cs.text == "This is a comment.");
}

TEST_CASE("parser parses not equal comparison", "[parser]") {
    Tokenizer t("If x is not equal to 3 then:\n    Say x.\nEnd if.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &ifstmt = std::get<IfStmt>(program[0]->node);
    auto &cond = std::get<BinaryExpr>(ifstmt.cond->node);
    CHECK(cond.op == BinOp::Ne);
}

TEST_CASE("parser parses greater than or equal to", "[parser]") {
    Tokenizer t("If x is greater than or equal to 3 then:\n    Say x.\nEnd if.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &ifstmt = std::get<IfStmt>(program[0]->node);
    auto &cond = std::get<BinaryExpr>(ifstmt.cond->node);
    CHECK(cond.op == BinOp::Ge);
}

TEST_CASE("parser handles mod operator", "[parser]") {
    Tokenizer t("Say 7 mod 3.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    auto &expr = std::get<BinaryExpr>(say.args[0]->node);
    CHECK(expr.op == BinOp::Mod);
}

TEST_CASE("parser handles Length of expression", "[parser]") {
    Tokenizer t("Say Length of \"hello\".");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<LengthExpr>(say.args[0]->node));
}

TEST_CASE("parser handles Call as expression", "[parser]") {
    Tokenizer t("Procedure double takes x:\n    Return x times 2.\nEnd procedure.\nSay Call double with 5 done.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    auto &say = std::get<SayStmt>(program[1]->node);
    CHECK(std::holds_alternative<CallExpr>(say.args[0]->node));
}

TEST_CASE("parser handles float literal", "[parser]") {
    Tokenizer t("Say 3.14.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<FloatLit>(say.args[0]->node));
}

TEST_CASE("parser handles unary minus", "[parser]") {
    Tokenizer t("Say minus 5.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<UnaryExpr>(say.args[0]->node));
    auto &unary = std::get<UnaryExpr>(say.args[0]->node);
    CHECK(unary.op == UnaryOp::Neg);
}

TEST_CASE("parser handles parenthesized expression", "[parser]") {
    Tokenizer t("Say (2 plus 3) times 4.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<BinaryExpr>(say.args[0]->node));
    auto &mul = std::get<BinaryExpr>(say.args[0]->node);
    CHECK(mul.op == BinOp::Mul);
    CHECK(std::holds_alternative<BinaryExpr>(mul.lhs->node));
    auto &add = std::get<BinaryExpr>(mul.lhs->node);
    CHECK(add.op == BinOp::Add);
}

TEST_CASE("parser handles power operator", "[parser]") {
    Tokenizer t("Say 2 to the power of 8.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<PowExpr>(say.args[0]->node));
}

TEST_CASE("parser handles math function call", "[parser]") {
    Tokenizer t("Say square root of 16.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 1);
    auto &say = std::get<SayStmt>(program[0]->node);
    CHECK(std::holds_alternative<MathCallExpr>(say.args[0]->node));
    auto &math = std::get<MathCallExpr>(say.args[0]->node);
    CHECK(math.func == "sqrt");
}

TEST_CASE("parser handles parameterless procedure", "[parser]") {
    Tokenizer t("Procedure greet:\n    Say \"hi\".\nEnd procedure.\nCall greet done.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<ProcedureStmt>(program[0]->node));
    auto &proc = std::get<ProcedureStmt>(program[0]->node);
    CHECK(proc.params.empty());
    CHECK(std::holds_alternative<CallStmt>(program[1]->node));
}

TEST_CASE("parser accepts Set synonyms assign and put", "[parser][synonyms]") {
    Tokenizer t("Put x to 1. Assign y to 2.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<SetStmt>(program[0]->node));
    CHECK(std::holds_alternative<SetStmt>(program[1]->node));
}

TEST_CASE("parser accepts Say synonyms show display write output", "[parser][synonyms]") {
    Tokenizer t("Show 1. Display 2. Write 3. Output 4.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 4);
    for (auto *s : program) CHECK(std::holds_alternative<SayStmt>(s->node));
}

TEST_CASE("parser accepts Increase and Decrease as AddStmt/SubStmt", "[parser][antonyms]") {
    Tokenizer t("Increase x by 3. Decrease y by 1.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<AddStmt>(program[0]->node));
    CHECK(std::holds_alternative<SubStmt>(program[1]->node));
}

TEST_CASE("parser accepts Unless and Until with negated conditions", "[parser][antonyms]") {
    Tokenizer t("Unless x is equal to 1 then: Say \"a\". End unless. Until x is equal to 2: Say \"b\". End until.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<IfStmt>(program[0]->node));
    CHECK(std::holds_alternative<WhileStmt>(program[1]->node));
    auto &iff = std::get<IfStmt>(program[0]->node);
    CHECK(std::holds_alternative<UnaryExpr>(iff.cond->node));
    auto &wh = std::get<WhileStmt>(program[1]->node);
    CHECK(std::holds_alternative<UnaryExpr>(wh.cond->node));
}

TEST_CASE("parser accepts Declare synonym create and Return synonym yield", "[parser][synonyms]") {
    Tokenizer t("Create n as integer with value 5. Procedure double takes n as integer returns integer: Yield n times 2. End procedure.");
    auto tokens = t.tokenize();
    Arena arena;
    Parser p(tokens, arena);
    auto program = p.parseProgram();
    REQUIRE(program.size() == 2);
    CHECK(std::holds_alternative<NativeDeclStmt>(program[0]->node));
    CHECK(std::holds_alternative<ProcedureStmt>(program[1]->node));
    auto &pnode = std::get<ProcedureStmt>(program[1]->node);
    REQUIRE(!pnode.body.empty());
    CHECK(std::holds_alternative<ReturnStmt>(pnode.body.back()->node));
}
