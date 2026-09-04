#include "parser.h"
#include "../lexer/alias_table.h"

const Token &Parser::peek(int ahead) const {
    size_t idx = pos_ + static_cast<size_t>(ahead);
    return idx < tokens_.size() ? tokens_[idx] : tokens_.back();
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
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Declare, Add, Append, Repeat, If, While, For, Call, Procedure)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "declare") return parseDeclare();
    if (t.text == "structure") return parseStructure();
    if (t.text == "union") return parseUnion();
    if (t.text == "enumeration") return parseEnumeration();
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
          "say, set/let/make, declare, add, subtract, read, append, replace, remove, repeat, if, while, for, call, procedure, return (see docs/grammar.md)");
}

Stmt *Parser::parseStmt() {
    const Token &t = peek();
    if (t.kind == TokKind::Comment) return parseComment();
    if (t.kind != TokKind::Ident) error("expected a sentence starting with a verb (Say, Set, Declare, Add, Append, Repeat, If, While, For, Call)");

    if (isSayKeyword(t.text)) return parseSay();
    if (isSetKeyword(t.text)) return parseSet();
    if (t.text == "declare") return parseDeclare();
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
          "say, set/let/make, declare, add, subtract, read, append, replace, remove, repeat, if, while, for, call, procedure, return (see docs/grammar.md)");
}

Stmt *Parser::parseSay() {
    int line = peek().line;
    advance();
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(SayStmt{expr}, line);
}

Stmt *Parser::parseSet() {
    int line = peek().line;
    advance();
    if (checkWord("value") && checkWordAt(1, "at")) {
        advance(); advance();
        Expr *pointer = parseExpr();
        expectWord("to");
        Expr *expr = parseExpr();
        expectDot();
        return arena_.makeStmt(StoreThroughStmt{pointer, expr}, line);
    }
    if (checkWord("element") && checkWordAt(1, "at")) {
        advance(); advance();
        Expr *index = parseExpr();
        expectWord("in");
        Expr *base = parseExpr();
        expectWord("to");
        Expr *expr = parseExpr();
        expectDot();
        return arena_.makeStmt(StoreElementStmt{index, base, expr}, line);
    }
    if (checkWord("member")) {
        advance();
        std::string member = expectIdentName();
        expectWord("of");
        Expr *base = parseExpr();
        expectWord("to");
        Expr *expr = parseExpr();
        expectDot();
        return arena_.makeStmt(StoreMemberStmt{std::move(member), base, expr}, line);
    }
    std::string name = expectIdentName();
    expectWord("to");
    Expr *expr = parseExpr();
    expectDot();
    return arena_.makeStmt(SetStmt{name, expr}, line);
}

Stmt *Parser::parseDeclare() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectWord("as");
    TypeSpec type = parseTypeSpec();
    Expr *initializer = nullptr;
    std::optional<AggregateInitializer> aggregateInitializer;
    if (checkWord("with")) {
        advance();
        if (checkWord("value")) {
            advance();
            initializer = parseExpr();
        } else if (checkWord("values")) {
            advance();
            AggregateInitializer aggregate;
            aggregate.kind = AggregateInitKind::Positional;
            if (checkWord("done")) error("with values needs at least one initializer value");
            for (;;) {
                aggregate.entries.push_back(AggregateInitEntry{"", 0, parseExpr()});
                if (checkWord("followed") && checkWordAt(1, "by")) {
                    advance(); advance();
                    continue;
                }
                break;
            }
            expectWord("done");
            aggregateInitializer = std::move(aggregate);
        } else if (checkWord("members")) {
            advance();
            AggregateInitializer aggregate;
            aggregate.kind = AggregateInitKind::Members;
            if (checkWord("done")) error("with members needs at least one member initializer");
            for (;;) {
                std::string member = expectIdentName();
                expectWord("as");
                aggregate.entries.push_back(AggregateInitEntry{std::move(member), 0, parseExpr()});
                if (checkWord("followed") && checkWordAt(1, "by")) {
                    advance(); advance();
                    continue;
                }
                break;
            }
            expectWord("done");
            aggregateInitializer = std::move(aggregate);
        } else if (checkWord("elements")) {
            advance();
            AggregateInitializer aggregate;
            aggregate.kind = AggregateInitKind::Elements;
            if (checkWord("done")) error("with elements needs at least one element initializer");
            for (;;) {
                expectWord("at");
                if (peek().kind != TokKind::Number || peek().num < 0) {
                    error("an aggregate element designator needs a non-negative whole-number literal index");
                }
                std::size_t index = static_cast<std::size_t>(advance().num);
                expectWord("as");
                aggregate.entries.push_back(AggregateInitEntry{"", index, parseExpr()});
                if (checkWord("followed") && checkWordAt(1, "by")) {
                    advance(); advance();
                    continue;
                }
                break;
            }
            expectWord("done");
            aggregateInitializer = std::move(aggregate);
        } else {
            error("expected value, values, members, or elements after with");
        }
    }
    expectDot();
    return arena_.makeStmt(NativeDeclStmt{name, std::move(type), initializer, std::move(aggregateInitializer)}, line);
}

Stmt *Parser::parseStructure() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectColon();

    std::vector<StructureField> fields;
    while (!(checkWord("end") && checkWordAt(1, "structure"))) {
        if (peek().kind == TokKind::Eof) {
            error("reached end of file while looking for \"End structure.\"");
        }
        expectWord("field");
        std::string fieldName = expectIdentName();
        expectWord("as");
        TypeSpec fieldType = parseTypeSpec();
        expectDot();
        fields.push_back(StructureField{std::move(fieldName), std::move(fieldType)});
    }
    advance(); advance();
    expectDot();
    return arena_.makeStmt(StructureStmt{std::move(name), std::move(fields)}, line);
}

Stmt *Parser::parseUnion() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectColon();

    std::vector<StructureField> fields;
    while (!(checkWord("end") && checkWordAt(1, "union"))) {
        if (peek().kind == TokKind::Eof) {
            error("reached end of file while looking for \"End union.\"");
        }
        expectWord("field");
        std::string fieldName = expectIdentName();
        expectWord("as");
        TypeSpec fieldType = parseTypeSpec();
        expectDot();
        fields.push_back(StructureField{std::move(fieldName), std::move(fieldType)});
    }
    advance(); advance();
    expectDot();
    return arena_.makeStmt(UnionStmt{std::move(name), std::move(fields)}, line);
}

Stmt *Parser::parseEnumeration() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectColon();

    std::vector<EnumeratorDef> enumerators;
    while (!(checkWord("end") && checkWordAt(1, "enumeration"))) {
        if (peek().kind == TokKind::Eof) {
            error("reached end of file while looking for \"End enumeration.\"");
        }
        expectWord("enumerator");
        std::string enumeratorName = expectIdentName();
        std::optional<long> explicitValue;
        if (checkWord("as")) {
            advance();
            bool negative = false;
            if (checkWord("minus")) {
                negative = true;
                advance();
            }
            if (peek().kind != TokKind::Number) {
                error("an Enumerator explicit value must be a whole-number literal");
            }
            long value = advance().num;
            explicitValue = negative ? -value : value;
        }
        expectDot();
        enumerators.push_back(EnumeratorDef{std::move(enumeratorName), explicitValue});
    }
    advance(); advance();
    expectDot();
    return arena_.makeStmt(EnumerationStmt{std::move(name), std::move(enumerators)}, line);
}

Stmt *Parser::parseAdd() {
    int line = peek().line;
    advance();
    Expr *expr = parseExpr();
    expectWord("to");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(AddStmt{expr, name}, line);
}

Stmt *Parser::parseSub() {
    int line = peek().line;
    advance();
    Expr *expr = parseExpr();
    expectWord("from");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(SubStmt{expr, name}, line);
}

Stmt *Parser::parseRead() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(ReadStmt{name}, line);
}

Stmt *Parser::parseReadFloat() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(ReadFloatStmt{name}, line);
}

Stmt *Parser::parseAppend() {
    int line = peek().line;
    advance();
    Expr *expr = parseExpr();
    expectWord("to");
    std::string name = expectIdentName();
    expectDot();
    return arena_.makeStmt(AppendStmt{expr, name}, line);
}

Stmt *Parser::parseReplaceItem() {
    int line = peek().line;
    advance();
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
    advance();
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
    advance();
    Expr *count = parseExpr();
    expectColon();
    auto body = parseBlockUntil("end", "repeat");
    return arena_.makeStmt(RepeatStmt{count, std::move(body)}, line);
}

Stmt *Parser::parseIf() {
    int line = peek().line;
    advance();
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
        advance();
        expectColon();
        while (!(checkWord("end") && checkWordAt(1, "if"))) {
            if (peek().kind == TokKind::Eof)
                error("reached end of file while looking for \"end if\" to close this block");
            elseBody.push_back(parseStmt());
        }
    }

    advance();
    advance();
    expectDot();
    return arena_.makeStmt(IfStmt{cond, std::move(thenBody), std::move(elseBody)}, line);
}

Stmt *Parser::parseWhile() {
    int line = peek().line;
    advance();
    Expr *cond = parseExpr();
    expectColon();
    auto body = parseBlockUntil("end", "while");
    return arena_.makeStmt(WhileStmt{cond, std::move(body)}, line);
}

Stmt *Parser::parseForEach() {
    int line = peek().line;
    advance();
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
    advance();
    Expr *expr = nullptr;
    if (peek().kind != TokKind::Dot) expr = parseExpr();
    expectDot();
    return arena_.makeStmt(ReturnStmt{expr}, line);
}

Stmt *Parser::parseCall() {
    int line = peek().line;
    advance();
    std::string name = expectIdentName();
    std::vector<Expr *> args;
    if (checkWord("with")) {
        advance();
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
    advance();
    std::string name = expectIdentName();
    std::vector<ProcedureParam> params;
    bool sawTypedParam = false;
    bool sawUntypedParam = false;

    if (checkWord("takes")) {
        advance();
        while (peek().kind != TokKind::Colon && !checkWord("returns")) {
            std::string paramName = expectIdentName();
            std::optional<TypeSpec> paramType;
            if (checkWord("as")) {
                advance();
                paramType = parseTypeSpec();
                sawTypedParam = true;
            } else {
                sawUntypedParam = true;
            }
            if (sawTypedParam && sawUntypedParam) {
                error("a Procedure cannot mix typed and untyped parameters; give every parameter an \"as <type>\" or none of them");
            }
            params.push_back(ProcedureParam{std::move(paramName), std::move(paramType)});
        }
    }

    std::optional<TypeSpec> returnType;
    if (checkWord("returns")) {
        advance();
        returnType = parseTypeSpec();
    }
    if (sawTypedParam && !returnType) {
        error("a Procedure with typed parameters must say what it returns, including \"returns void\"");
    }
    if (sawUntypedParam && returnType) {
        error("a typed Procedure return requires typed parameters; either add \"as <type>\" to every parameter or remove \"returns\"");
    }

    expectColon();
    auto body = parseBlockUntil("end", "procedure");
    return arena_.makeStmt(ProcedureStmt{name, std::move(params), std::move(returnType), std::move(body)}, line);
}

std::vector<Stmt *> Parser::parseBlockUntil(const std::string &w1, const std::string &w2) {
    std::vector<Stmt *> body;
    while (!(checkWord(w1) && checkWordAt(1, w2))) {
        if (peek().kind == TokKind::Eof)
            error("reached end of file while looking for \"" + w1 + " " + w2 + ".\" to close this block");
        body.push_back(parseStmt());
    }
    advance();
    advance();
    expectDot();
    return body;
}

TypeSpec Parser::parseTypeSpec() {
    if (checkWord("enumeration")) {
        advance();
        std::string tag = expectIdentName();
        TypeSpec type{TypeSpecKind::Enumeration};
        type.tag = std::move(tag);
        return type;
    }
    if (checkWord("union")) {
        advance();
        std::string tag = expectIdentName();
        TypeSpec type{TypeSpecKind::Union};
        type.tag = std::move(tag);
        return type;
    }
    if (checkWord("structure")) {
        advance();
        std::string tag = expectIdentName();
        TypeSpec type{TypeSpecKind::Structure};
        type.tag = std::move(tag);
        return type;
    }
    if (checkWord("array") && checkWordAt(1, "of")) {
        advance(); advance();
        TypeSpec element = parseTypeSpec();
        expectWord("with");
        expectWord("length");
        if (peek().kind != TokKind::Number || peek().num <= 0) {
            error("a fixed native array length must be a positive whole-number literal");
        }
        std::size_t bound = static_cast<std::size_t>(advance().num);
        return TypeSpec{TypeSpecKind::Array, std::make_shared<TypeSpec>(std::move(element)), bound};
    }
    if (checkWord("pointer") && checkWordAt(1, "to")) {
        advance(); advance();
        TypeSpec pointee = parseTypeSpec();
        return TypeSpec{TypeSpecKind::Pointer, std::make_shared<TypeSpec>(std::move(pointee))};
    }
    if (checkWord("void")) { advance(); return TypeSpec{TypeSpecKind::Void}; }
    if (checkWord("boolean")) { advance(); return TypeSpec{TypeSpecKind::Boolean}; }
    if (checkWord("character")) { advance(); return TypeSpec{TypeSpecKind::Character}; }
    if (checkWord("signed") && checkWordAt(1, "character")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::SignedCharacter};
    }
    if (checkWord("unsigned") && checkWordAt(1, "character")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::UnsignedCharacter};
    }
    if (checkWord("short") && checkWordAt(1, "integer")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::ShortInteger};
    }
    if (checkWord("unsigned") && checkWordAt(1, "short") && checkWordAt(2, "integer")) {
        advance(); advance(); advance(); return TypeSpec{TypeSpecKind::UnsignedShortInteger};
    }
    if (checkWord("integer")) { advance(); return TypeSpec{TypeSpecKind::Integer}; }
    if (checkWord("unsigned") && checkWordAt(1, "integer")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::UnsignedInteger};
    }
    if (checkWord("long") && checkWordAt(1, "long") && checkWordAt(2, "integer")) {
        advance(); advance(); advance(); return TypeSpec{TypeSpecKind::LongLongInteger};
    }
    if (checkWord("unsigned") && checkWordAt(1, "long") && checkWordAt(2, "long") && checkWordAt(3, "integer")) {
        advance(); advance(); advance(); advance(); return TypeSpec{TypeSpecKind::UnsignedLongLongInteger};
    }
    if (checkWord("long") && checkWordAt(1, "integer")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::LongInteger};
    }
    if (checkWord("unsigned") && checkWordAt(1, "long") && checkWordAt(2, "integer")) {
        advance(); advance(); advance(); return TypeSpec{TypeSpecKind::UnsignedLongInteger};
    }
    if (checkWord("float")) { advance(); return TypeSpec{TypeSpecKind::Float}; }
    if (checkWord("decimal")) { advance(); return TypeSpec{TypeSpecKind::Decimal}; }
    if (checkWord("long") && checkWordAt(1, "decimal")) {
        advance(); advance(); return TypeSpec{TypeSpecKind::LongDecimal};
    }

    error("expected a C type such as \"integer\", \"pointer to integer\", \"array of integer with length 4\", \"structure point\", \"union value\", \"enumeration color\", \"unsigned long integer\", \"character\", \"float\", or \"decimal\"");
}

Expr *Parser::parseExpr() { return parseOr(); }

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
        advance();
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
        if (op == BinOp::Div) advance();
        advance();
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
    if (checkWord("address") && checkWordAt(1, "of")) {
        int line = peek().line;
        advance(); advance();
        std::string name = expectIdentName();
        return arena_.makeExpr(AddressOfExpr{name}, line);
    }
    if (checkWord("value") && checkWordAt(1, "at")) {
        int line = peek().line;
        advance(); advance();
        Expr *pointer = parsePrimary();
        return arena_.makeExpr(DerefExpr{pointer}, line);
    }
    if (checkWord("length") && checkWordAt(1, "of")) {
        int line = peek().line;
        advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(LengthExpr{operand}, line);
    }
    if (checkWord("size") && checkWordAt(1, "of") && checkWordAt(2, "type")) {
        int line = peek().line;
        advance(); advance(); advance();
        return arena_.makeExpr(SizeOfTypeExpr{parseTypeSpec()}, line);
    }
    if (checkWord("size") && checkWordAt(1, "of")) {
        int line = peek().line;
        advance(); advance();
        Expr *operand = parsePrimary();
        return arena_.makeExpr(SizeOfExpr{operand}, line);
    }
    if (checkWord("alignment") && checkWordAt(1, "of") && checkWordAt(2, "type")) {
        int line = peek().line;
        advance(); advance(); advance();
        return arena_.makeExpr(AlignOfTypeExpr{parseTypeSpec()}, line);
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
    if (checkWord("enumerator")) {
        int line = peek().line;
        advance();
        std::string name = expectIdentName();
        expectWord("of");
        std::string enumeration = expectIdentName();
        return arena_.makeExpr(EnumeratorExpr{std::move(name), std::move(enumeration)}, line);
    }
    if (checkWord("member")) {
        int line = peek().line;
        advance();
        std::string name = expectIdentName();
        expectWord("of");
        Expr *base = parsePrimary();
        return arena_.makeExpr(MemberExpr{std::move(name), base}, line);
    }
    if (checkWord("element") && checkWordAt(1, "at")) {
        int line = peek().line;
        advance(); advance();
        Expr *index = parseExpr();
        expectWord("in");
        Expr *base = parsePrimary();
        return arena_.makeExpr(ElementExpr{index, base}, line);
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
        advance();
        std::string name = expectIdentName();
        std::vector<Expr *> args;
        if (checkWord("with")) {
            advance();
            while (!checkWord("done")) {
                if (peek().kind == TokKind::Eof) error("reached end of file while looking for \"done\" to close this call");
                args.push_back(parseExpr());
            }
        }
        expectWord("done");
        return arena_.makeExpr(CallExpr{name, std::move(args)}, line);
    }
    if (t.kind == TokKind::Ident) { advance(); return arena_.makeExpr(VarRef{t.text}, t.line); }
    error("expected a number, a decimal, a string, a name, true, false, minus, Address of, Value at, Length of, Size of, Alignment of type, List with, Empty list of, Item at, or a math function here");
}
