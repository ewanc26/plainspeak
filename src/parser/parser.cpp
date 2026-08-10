#include "parser.h"
#include "../lexer/alias_table.h"

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
    if (t.text == "subtract") return parseSub();
    if (t.text == "read") return parseRead();
    if (t.text == "comment") return parseComment();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "call") return parseCall();
    if (t.text == "procedure") return parseProcedure();
    if (t.text == "return") return parseReturn();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, subtract, read, comment, repeat, if, while, call, procedure, return (see docs/grammar.md)");
}

Stmt *Parser::parseStmt() {
    const Token &t = peek();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Add, Repeat, If, While, Call)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "add") return parseAdd();
    if (t.text == "subtract") return parseSub();
    if (t.text == "read") return parseRead();
    if (t.text == "comment") return parseComment();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "call") return parseCall();
    if (t.text == "procedure") return parseProcedure();
    if (t.text == "return") return parseReturn();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, subtract, read, comment, repeat, if, while, call, procedure, return (see docs/grammar.md)");
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

Stmt *Parser::parseSub() {
    int line = peek().line;
    advance(); // subtract
    Expr *expr = parseExpr();
    expectWord("from");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(SubStmt{expr, name}, line);
}

Stmt *Parser::parseRead() {
    int line = peek().line;
    advance(); // read
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(ReadStmt{name}, line);
}

Stmt *Parser::parseComment() {
    int line = peek().line;
    advance(); // comment
    std::string text;
    while (peek().kind != TokKind::Dot && peek().kind != TokKind::Eof) {
        if (!text.empty()) text += " ";
        text += peek().text;
        advance();
    }
    expectDot();
    return arena_.makeStmt(CommentStmt{text}, line);
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

Stmt *Parser::parseReturn() {
    int line = peek().line;
    advance(); // return
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(ReturnStmt{expr}, line);
}

Stmt *Parser::parseCall() {
    int line = peek().line;
    advance(); // call
    std::string name = expectIdentName();
    expectWord("with");
    std::vector<Expr *> args;
    while (!checkWord("done")) {
        args.push_back(parseExpr());
    }
    advance(); // done
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
    return parseOr();
}

Expr *Parser::parseOr() {
    Expr *lhs = parseAnd();
    while (checkWord("or")) {
        int line = peek().line;
        advance();
        Expr *rhs = parseAnd();
        lhs = arena_.makeExpr(BinaryExpr{BinOp::Or, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parseAnd() {
    Expr *lhs = parseNot();
    while (checkWord("and")) {
        int line = peek().line;
        advance();
        Expr *rhs = parseNot();
        lhs = arena_.makeExpr(BinaryExpr{BinOp::And, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parseNot() {
    if (checkWord("not")) {
        int line = peek().line;
        advance();
        Expr *rhs = parseNot();
        return arena_.makeExpr(UnaryExpr{UnaryOp::Not, rhs}, line);
    }
    return parseComparison();
}

Expr *Parser::parseComparison() {
    Expr *lhs = parseAdditive();
    if (checkWord("is")) {
        int line = peek().line;
        advance(); // is
        BinOp op;
        if (checkWord("greater") && checkWordAt(1, "than") && checkWordAt(2, "or") && checkWordAt(3, "equal") && checkWordAt(4, "to")) { op = BinOp::Ge; advance(); advance(); advance(); advance(); advance(); }
        else if (checkWord("less") && checkWordAt(1, "than") && checkWordAt(2, "or") && checkWordAt(3, "equal") && checkWordAt(4, "to")) { op = BinOp::Le; advance(); advance(); advance(); advance(); advance(); }
        else if (checkWord("greater") && checkWordAt(1, "than")) { op = BinOp::Gt; advance(); advance(); }
        else if (checkWord("less") && checkWordAt(1, "than")) { op = BinOp::Lt; advance(); advance(); }
        else if (checkWord("equal") && checkWordAt(1, "to")) { op = BinOp::Eq; advance(); advance(); }
        else if (checkWord("not") && checkWordAt(1, "equal") && checkWordAt(2, "to")) { op = BinOp::Ne; advance(); advance(); advance(); }
        else error("expected \"greater than\", \"less than\", \"equal to\", \"not equal to\", \"greater than or equal to\", or \"less than or equal to\" after \"is\"");
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
    while (checkWord("times") || (checkWord("divided") && checkWordAt(1, "by")) || checkWord("mod")) {
        int line = peek().line;
        BinOp op = checkWord("times") ? BinOp::Mul : (checkWord("mod") ? BinOp::Mod : BinOp::Div);
        if (op == BinOp::Div) advance(); // divided
        advance(); // times, by, or mod
        Expr *rhs = parsePrimary();
        lhs = arena_.makeExpr(BinaryExpr{op, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parsePrimary() {
    const Token &t = peek();
    if (t.kind == TokKind::Number) { advance(); return arena_.makeExpr(IntLit{t.num}, t.line); }
    if (t.kind == TokKind::String) { advance(); return arena_.makeExpr(StringLit{t.text}, t.line); }
    if (checkWord("true"))  { advance(); return arena_.makeExpr(BoolLit{true}, t.line); }
    if (checkWord("false")) { advance(); return arena_.makeExpr(BoolLit{false}, t.line); }
    if (checkWord("length") && checkWordAt(1, "of")) {
        int line = peek().line;
        advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(LengthExpr{operand}, line);
    }
    if (checkWord("call")) {
        int line = peek().line;
        advance(); // call
        std::string name = expectIdentName();
        expectWord("with");
        std::vector<Expr *> args;
        while (!checkWord("done")) {
            args.push_back(parseExpr());
        }
        advance(); // done
        return arena_.makeExpr(CallExpr{name, std::move(args)}, line);
    }
    if (t.kind == TokKind::Ident)  { advance(); return arena_.makeExpr(VarRef{t.text}, t.line); }
    error("expected a number, a string, a name, true, false, or length of here");
}
