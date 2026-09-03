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
    if (t.kind == TokKind::Comment) return parseComment();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Add, Append, Repeat, If, While, For, Call, Procedure)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "add") return parseAdd();
    if (t.text == "subtract") return parseSub();
    if (t.text == "readfloat") return parseReadFloat();
    if (t.text == "read") return parseRead();
    if (t.text == "append") return parseAppend();
    if (t.text == "replace") return parseReplaceItem();
    if (t.text == "remove") return parseRemoveItem();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "for") return parseForEach();
    if (t.text == "call") return parseCall();
    if (t.text == "procedure") return parseProcedure();
    if (t.text == "return") return parseReturn();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, subtract, read, append, replace, remove, repeat, if, while, for, call, procedure, return (see docs/grammar.md)");
}

Stmt *Parser::parseStmt() {
    const Token &t = peek();
    if (t.kind == TokKind::Comment) return parseComment();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Add, Append, Repeat, If, While, For, Call)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "add") return parseAdd();
    if (t.text == "subtract") return parseSub();
    if (t.text == "readfloat") return parseReadFloat();
    if (t.text == "read") return parseRead();
    if (t.text == "append") return parseAppend();
    if (t.text == "replace") return parseReplaceItem();
    if (t.text == "remove") return parseRemoveItem();
    if (t.text == "repeat") return parseRepeat();
    if (t.text == "if") return parseIf();
    if (t.text == "while") return parseWhile();
    if (t.text == "for") return parseForEach();
    if (t.text == "call") return parseCall();
    if (t.text == "procedure") return parseProcedure();
    if (t.text == "return") return parseReturn();

    error("I don't know the verb \"" + t.text + "\" — expected one of: "
          "say, set/let/make, add, subtract, read, append, replace, remove, repeat, if, while, for, call, procedure, return (see docs/grammar.md)");
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

Stmt *Parser::parseReadFloat() {
    int line = peek().line;
    advance(); // readfloat
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(ReadFloatStmt{name}, line);
}

Stmt *Parser::parseAppend() {
    int line = peek().line;
    advance(); // append
    Expr *expr = parseExpr();
    expectWord("to");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(AppendStmt{expr, name}, line);
}

Stmt *Parser::parseReplaceItem() {
    int line = peek().line;
    advance(); // replace
    expectWord("item");
    expectWord("at");
    Expr *index = parseExpr();
    expectWord("in");
    std::string name = expectIdentName();
    expectWord("with");
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(ReplaceItemStmt{index, name, expr}, line);
}

Stmt *Parser::parseRemoveItem() {
    int line = peek().line;
    advance(); // remove
    expectWord("item");
    expectWord("at");
    Expr *index = parseExpr();
    expectWord("from");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(RemoveItemStmt{index, name}, line);
}

Stmt *Parser::parseComment() {
    int line = peek().line;
    std::string text = advance().text;
    return arena_.makeStmt(CommentStmt{std::move(text)}, line);
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

Stmt *Parser::parseForEach() {
    int line = peek().line;
    advance(); // for
    expectWord("each");
    std::string itemName = expectIdentName();
    expectWord("in");
    Expr *list = parseExpr();
    expectColon();
    auto body = parseBlockUntil("end", "for");
    return arena_.makeStmt(ForEachStmt{itemName, list, std::move(body)}, line);
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
    std::vector<Expr *> args;
    if (checkWord("with")) {
        advance(); // with
        while (!checkWord("done")) {
            if (peek().kind == TokKind::Eof) error("reached end of file while looking for \"done\" to close this call");
            args.push_back(parseExpr());
        }
    }
    expectWord("done");
    expectDot();
    return arena_.makeStmt(CallStmt{name, std::move(args)}, line);
}

Stmt *Parser::parseProcedure() {
    int line = peek().line;
    advance(); // procedure
    std::string name = expectIdentName();
    std::vector<std::string> params;
    if (checkWord("takes")) {
        advance(); // takes
        params.push_back(expectIdentName());
        while (peek().kind == TokKind::Ident) {
            params.push_back(expectIdentName());
        }
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
    Expr *lhs = parsePower();
    while (checkWord("times") || (checkWord("divided") && checkWordAt(1, "by")) || checkWord("mod") || checkWord("modulo")) {
        int line = peek().line;
        BinOp op = checkWord("times") ? BinOp::Mul : (checkWord("mod") || checkWord("modulo") ? BinOp::Mod : BinOp::Div);
        if (op == BinOp::Div) advance(); // divided
        advance(); // times, by, or mod
        Expr *rhs = parsePower();
        lhs = arena_.makeExpr(BinaryExpr{op, lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parsePower() {
    Expr *lhs = parsePrimary();
    while (checkWord("to") && checkWordAt(1, "the") && checkWordAt(2, "power") && checkWordAt(3, "of")) {
        int line = peek().line;
        advance(); advance(); advance(); advance();
        Expr *rhs = parsePrimary();
        lhs = arena_.makeExpr(PowExpr{lhs, rhs}, line);
    }
    return lhs;
}

Expr *Parser::parsePrimary() {
    const Token &t = peek();
    if (t.kind == TokKind::Number) { advance(); return arena_.makeExpr(IntLit{t.num}, t.line); }
    if (t.kind == TokKind::Float) { advance(); return arena_.makeExpr(FloatLit{t.fval}, t.line); }
    if (t.kind == TokKind::String) { advance(); return arena_.makeExpr(StringLit{t.text}, t.line); }
    if (checkWord("true"))  { advance(); return arena_.makeExpr(BoolLit{true}, t.line); }
    if (checkWord("false")) { advance(); return arena_.makeExpr(BoolLit{false}, t.line); }
    if (checkWord("minus")) {
        int line = peek().line;
        advance();
        Expr *rhs = parsePrimary();
        return arena_.makeExpr(UnaryExpr{UnaryOp::Neg, rhs}, line);
    }
    if (checkWord("length") && checkWordAt(1, "of")) {
        int line = peek().line;
        advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(LengthExpr{operand}, line);
    }
    if (checkWord("list") && checkWordAt(1, "with")) {
        int line = peek().line;
        advance(); advance();
        if (checkWord("done")) error("a list introduced with \"List with\" needs at least one item; use \"Empty list of ...\" for an empty list");
        std::vector<Expr *> items;
        items.push_back(parseExpr());
        while (checkWord("followed") && checkWordAt(1, "by")) {
            advance(); advance();
            items.push_back(parseExpr());
        }
        expectWord("done");
        return arena_.makeExpr(ListExpr{std::move(items)}, line);
    }
    if (checkWord("empty") && checkWordAt(1, "list") && checkWordAt(2, "of")) {
        int line = peek().line;
        advance(); advance(); advance();
        ListElementKind elementKind;
        if (checkWord("numbers")) elementKind = ListElementKind::Number;
        else if (checkWord("decimals")) elementKind = ListElementKind::Decimal;
        else if (checkWord("strings")) elementKind = ListElementKind::String;
        else error("expected \"numbers\", \"decimals\", or \"strings\" after \"Empty list of\"");
        advance();
        return arena_.makeExpr(EmptyListExpr{elementKind}, line);
    }
    if (checkWord("item") && checkWordAt(1, "at")) {
        int line = peek().line;
        advance(); advance();
        Expr *index = parseExpr();
        expectWord("in");
        Expr *list = parsePrimary();
        return arena_.makeExpr(ItemExpr{index, list}, line);
    }
    if (checkWord("square") && checkWordAt(1, "root") && checkWordAt(2, "of")) {
        int line = peek().line;
        advance(); advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(MathCallExpr{"sqrt", operand}, line);
    }
    if (checkWord("absolute") && checkWordAt(1, "value") && checkWordAt(2, "of")) {
        int line = peek().line;
        advance(); advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(MathCallExpr{"abs", operand}, line);
    }
    {
        static const char *mathFns[] = {"sine", "cosine", "tangent", "sqrt", "log", "abs", "floor", "ceil"};
        for (const char *fn : mathFns) {
            if (checkWord(fn) && checkWordAt(1, "of")) {
                int line = peek().line;
                advance(); advance();
                Expr *operand = parsePrimary();
                return arena_.makeExpr(MathCallExpr{fn, operand}, line);
            }
        }
    }
    if (peek().kind == TokKind::LParen) {
        advance();
        Expr *inner = parseExpr();
        if (peek().kind != TokKind::RParen) error("expected \")\"");
        advance();
        return inner;
    }
    if (checkWord("call")) {
        int line = peek().line;
        advance(); // call
        std::string name = expectIdentName();
        std::vector<Expr *> args;
        if (checkWord("with")) {
            advance(); // with
            while (!checkWord("done")) {
                if (peek().kind == TokKind::Eof) error("reached end of file while looking for \"done\" to close this call");
                args.push_back(parseExpr());
            }
        }
        expectWord("done");
        return arena_.makeExpr(CallExpr{name, std::move(args)}, line);
    }
    if (t.kind == TokKind::Ident) { advance(); return arena_.makeExpr(VarRef{t.text}, t.line); }
    error("expected a number, a decimal, a string, a name, true, false, minus, Length of, List with, Empty list of, Item at, or a math function here");
}
