#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "../src/lexer/tokenizer.h"

using Catch::Approx;

TEST_CASE("tokenizer splits identifiers and keywords", "[lexer]") {
    Tokenizer t("say hello world");
    auto tokens = t.tokenize();
    REQUIRE(tokens.size() == 4);
    CHECK(tokens[0].kind == TokKind::Ident);
    CHECK(tokens[0].text == "say");
    CHECK(tokens[1].text == "hello");
    CHECK(tokens[2].text == "world");
    CHECK(tokens[3].kind == TokKind::Eof);
}

TEST_CASE("tokenizer handles numbers", "[lexer]") {
    Tokenizer t("42");
    auto tokens = t.tokenize();
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].kind == TokKind::Number);
    CHECK(tokens[0].num == 42);
}

TEST_CASE("tokenizer handles float literals", "[lexer]") {
    Tokenizer t("3.14");
    auto tokens = t.tokenize();
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].kind == TokKind::Float);
    CHECK(tokens[0].fval == Approx(3.14));
}

TEST_CASE("tokenizer handles parentheses", "[lexer]") {
    Tokenizer t("(1 plus 2)");
    auto tokens = t.tokenize();
    REQUIRE(tokens.size() == 6);
    CHECK(tokens[0].kind == TokKind::LParen);
    CHECK(tokens[1].kind == TokKind::Number);
    CHECK(tokens[4].kind == TokKind::RParen);
    CHECK(tokens[5].kind == TokKind::Eof);
}

TEST_CASE("tokenizer handles strings with escapes", "[lexer]") {
    Tokenizer t("\"hello\\nworld\"");
    auto tokens = t.tokenize();
    REQUIRE(tokens.size() == 2);
    CHECK(tokens[0].kind == TokKind::String);
    CHECK(tokens[0].text == "hello\nworld");
}

TEST_CASE("tokenizer is case-insensitive", "[lexer]") {
    Tokenizer t("Say PRINT Set");
    auto tokens = t.tokenize();
    CHECK(tokens[0].text == "say");
    CHECK(tokens[1].text == "print");
    CHECK(tokens[2].text == "set");
}
