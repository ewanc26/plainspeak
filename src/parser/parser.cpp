#include "parser.h"

const Token &Parser::peek(int ahead) const {
    size_t idx = pos_ + static_cast<size_t>(ahead);
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back(); // Eof
}

const Token &Parser::advance() {
    const Token &t = tokens_[pos_];
    if (pos_ + 1 < tokens_.size()) pos_++;
    return t;
}

bool Parser::checkWord(const std::string &word) const { return checkWordAt(0, word); }

bool Parser::checkWordAt(int ahead, const std::string &word) const {
    const Token &t = peek(ahead);
    return t.kind == TokKind::Ident && t.text == word;
}

void Parser::expectWord(const std::string &word) {
    if (!checkWord(word)) {
        error("expected \"" + word + "\" but found \"" +
              (peek().kind == TokKind::Eof ? "end of file" : peek().text) + "\"");
    }
    advance();
}

void Parser::expectDot() {
    if (peek().kind != TokKind::Dot) error("expected \".\" to end the sentence");
    advance();
}

void Parser::expectColon() {
    if (peek().kind != TokKind::Colon) error("expected \":\" to start a block");
    advance();
}

std::string Parser::expectIdentName() {
    if (peek().kind != TokKind::Ident) error("expected a name here");
    return advance().text;
}

void Parser::error(const std::string &msg) const {
    throw ParseError("line " + std::to_string(peek().line) + ": " + msg);
}

// alias resolution lives here, not in the lexer — see AGENTS.md §4.2:
// each of these is a flat, exact word match, never fuzzy/partial.
static bool isSetKeyword(const std::string &w) { return w == "set" || w == "let" || w == "make"; }
static bool isSayKeyword(const std::string &w) { return w == "say" || w == "print"; }

std::vector<Stmt *> Parser::parseProgram() {
    std::vector<Stmt *> stmts;
    while (peek().kind != TokKind::Eof) stmts.push_back(parseTopLevelStmt());
    return stmts;
}

Stmt *Parser::parseTopLevelStmt() {
    const Token &t = peek();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Add, Repeat, If, While, Call, Procedure)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "add") return parseAdd();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "call") return parseCall();
    if (t.text == "procedure") return parseProcedure();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, repeat, if, while, call, procedure (see docs/grammar.md)");
}

Stmt *Parser::parseStmt() {
    const Token &t = peek();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Add, Repeat, If, While, Call)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "add") return parseAdd();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "call") return parseCall();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, repeat, if, while, call (see docs/grammar.md)");
}

Stmt *Parser::parseSay() {
    int line = peek().line;
    advance(); // say/print
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(SayStmt{expr}, line);
}

Stmt *Parser::parseSet() {
    int line = peek().line;
    advance(); // set/let/make
    std::string name = expectIdentName();
    expectWord("to");
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(SetStmt{name, expr}, line);
}

Stmt *Parser::parseAdd() {
    int line = peek().line;
    advance(); // add
    Expr *expr = parseExpr();
    expectWord("to");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(AddStmt{expr, name}, line);
}

Stmt *Parser::parseRepeat() {
    int line = peek().line;
    advance(); // repeat
    Expr *count = parseExpr();
    expectColon();
    auto body = parseBlockUntil("end", "repeat");
    return arena_.makeStmt(RepeatStmt{count, std::move(body)}, line);
}

Stmt *Parser::parseIf() {
    int line = peek().line;
    advance(); // if
    Expr *cond = parseExpr();
    expectWord("then");
    expectColon();

    std::vector<Stmt *> thenBody;
    while (!(checkWord("else") || (checkWord("end") && checkWordAt(1, "if")))) {
        if (peek().kind == TokKind::Eof)
            error("reached end of file while looking for \"else\" or \"end if\" to close this block");
        thenBody.push_back(parseStmt());
    }

    std::vector<Stmt *> elseBody;
    if (checkWord("else")) {
        advance(); // else
        expectColon();
        while (!(checkWord("end") && checkWordAt(1, "if"))) {
            if (peek().kind == TokKind::Eof)
                error("reached end of file while looking for \"end if\" to close this block");
            elseBody.push_back(parseStmt());
        }
    }

    advance(); // end
    advance(); // if
    expectDot();
    return arena_.makeStmt(IfStmt{cond, std::move(thenBody), std::move(elseBody)}, line);
}

Stmt *Parser::parseWhile() {
    int line = peek().line;
    advance(); // while
    Expr *cond = parseExpr();
    expectColon();
    auto body = parseBlockUntil("end", "while");
    return arena_.makeStmt(WhileStmt{cond, std::move(body)}, line);
}

Stmt *Parser::parseCall() {
    int line = peek().line;
    advance(); // call
    std::string name = expectIdentName();
    expectWord("with");
    std::vector<Expr *> args;
    args.push_back(parseExpr());
    while (peek().kind != TokKind::Dot) {
        args.push_back(parseExpr());
    }
    expectDot();
    return arena_.makeStmt(CallStmt{name, std::move(args)}, line);
}

Stmt *Parser::parseProcedure() {
    int line = peek().line;
    advance(); // procedure
    std::string name = expectIdentName();
    expectWord("takes");
    std::vector<std::string> params;
    params.push_back(expectIdentName());
    while (peek().kind == TokKind::Ident && peek().text != ":") {
        params.push_back(expectIdentName());
    }
    expectColon();
    auto body = parseBlockUntil("end", "procedure");
    return arena_.makeStmt(ProcedureStmt{name, std::move(params), std::move(body)}, line);
}

std::vector<Stmt *> Parser::parseBlockUntil(const std::string &w1, const std::string &w2) {
    std::vector<Stmt *> body;
    while (!(checkWord(w1) && checkWordAt(1, w2))) {
        if (peek().kind == TokKind::Eof)
            error("reached end of file while looking for \"" + w1 + " " + w2 + ".\" to close this block");
        body.push_back(parseStmt());
    }
    advance(); // w1
    advance(); // w2
    expectDot();
    return body;
}

Expr *Parser::parseExpr() {
    Expr *lhs = parseAdditive();
    if (checkWord("is")) {
        int line = peek().line;
        advance(); // is
        BinOp op;
        if (checkWord("greater") && checkWordAt(1, "than")) { op = BinOp::Gt; advance(); advance(); }
        else if (checkWord("less") && checkWordAt(1, "than")) { op = BinOp::Lt; advance(); advance(); }
        else if (checkWord("equal") && checkWordAt(1, "to")) { op = BinOp::Eq; advance(); advance(); }
        else error("expected \"greater than\", \"less than\", or \"equal to\" after \"is\"");
        Expr *rhs = parseAdditive();
        return arena_.makeExpr(BinaryExpr{op, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parseAdditive() {
    Expr *lhs = parseMultiplicative();
    while (checkWord("plus") || checkWord("minus")) {
        int line = peek().line;
        BinOp op = checkWord("plus") ? BinOp::Add : BinOp::Sub;
        advance();
        Expr *rhs = parseMultiplicative();
        lhs = arena_.makeExpr(BinaryExpr{op, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parseMultiplicative() {
    Expr *lhs = parsePrimary();
    while (checkWord("times") || (checkWord("divided") && checkWordAt(1, "by"))) {
        int line = peek().line;
        BinOp op = checkWord("times") ? BinOp::Mul : BinOp::Div;
        if (op == BinOp::Div) advance(); // divided
        advance(); // times or by
        Expr *rhs = parsePrimary();
        lhs = arena_.makeExpr(BinaryExpr{op, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parsePrimary() {
    const Token &t = peek();
    if (t.kind == TokKind::Number) { advance(); return arena_.makeExpr(IntLit{t.num}, t.line); }
    if (t.kind == TokKind::String) { advance(); return arena_.makeExpr(StringLit{t.text}, t.line); }
    if (t.kind == TokKind::Ident)  { advance(); return arena_.makeExpr(VarRef{t.text}, t.line); }
    error("expected a number, a string, or a name here");
}
