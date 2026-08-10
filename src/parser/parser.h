#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "../ast/ast.h"
#include "../lexer/tokenizer.h"

// Thrown on any sentence that doesn't match a known pattern. main.cpp
// catches this and prints it as the user-facing diagnostic — see
// AGENTS.md §9 for the target error style (v0 keeps this plain; upgrading
// to the "literal-minded listener" format with source spans is tracked in
// docs/errors.md).
struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Recursive-descent parser, one function per grammar rule in
// docs/grammar.md — keep that file and this one in sync (AGENTS.md §5).
class Parser {
public:
    Parser(std::vector<Token> tokens, Arena &arena)
        : tokens_(std::move(tokens)), arena_(arena) {}

    std::vector<Stmt *> parseProgram();

private:
    std::vector<Token> tokens_;
    Arena &arena_;
    size_t pos_ = 0;

    const Token &peek(int ahead = 0) const;
    const Token &advance();
    bool checkWord(const std::string &word) const; // case-insensitive Ident match
    bool checkWordAt(int ahead, const std::string &word) const;
    void expectWord(const std::string &word);
    void expectDot();
    void expectColon();
    std::string expectIdentName();
    [[noreturn]] void error(const std::string &msg) const;

    Stmt *parseStmt();
    Stmt *parseSay();
    Stmt *parseSet();
    Stmt *parseAdd();
    Stmt *parseRepeat();
    Stmt *parseIf();
    Stmt *parseWhile();
    std::vector<Stmt *> parseBlockUntil(const std::string &w1, const std::string &w2);

    Expr *parseExpr();        // comparison level (lowest precedence)
    Expr *parseAdditive();    // "plus"
    Expr *parsePrimary();     // literal / identifier
};
