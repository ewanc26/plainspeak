#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include "../ast/ast.h"
#include "../lexer/tokenizer.h"

struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

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
    bool checkWord(const std::string &word) const;
    bool checkWordAt(int ahead, const std::string &word) const;
    void expectWord(const std::string &word);
    void expectDot();
    void expectColon();
    std::string expectIdentName();
    [[noreturn]] void error(const std::string &msg) const;

    Stmt *parseStmt();
    Stmt *parseSay();
    Stmt *parseSet();
    Stmt *parseDeclare();
    Stmt *parseStructure();
    Stmt *parseUnion();
    Stmt *parseEnumeration();
    Stmt *parseAdd();
    Stmt *parseSub();
    Stmt *parseRead();
    Stmt *parseReadFloat();
    Stmt *parseAppend();
    Stmt *parseReplaceItem();
    Stmt *parseRemoveItem();
    Stmt *parseComment();
    Stmt *parseRepeat();
    Stmt *parseIf();
    Stmt *parseWhile();
    Stmt *parseDoWhile();
    Stmt *parseForEach();
    Stmt *parseCall();
    Stmt *parseReturn();
    Stmt *parseTopLevelStmt();
    Stmt *parseProcedure();
    std::vector<Stmt *> parseBlockUntil(const std::string &w1, const std::string &w2);

    TypeSpec parseTypeSpec();

    Expr *parseExpr();
    Expr *parseOr();
    Expr *parseAnd();
    Expr *parseBitwiseOr();
    Expr *parseBitwiseXor();
    Expr *parseBitwiseAnd();
    Expr *parseNot();
    Expr *parseComparison();
    Expr *parseShift();
    Expr *parseAdditive();
    Expr *parseMultiplicative();
    Expr *parsePower();
    Expr *parsePrimary();
};
