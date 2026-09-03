#pragma once
#include <string>
#include <vector>

enum class TokKind { Ident, Number, Float, String, Comment, Dot, Colon, LParen, RParen, Eof };

struct Token {
    TokKind kind;
    std::string text; // for Ident/String/Comment: the value. Idents are lowercased
                      // for keyword matching; comments preserve their source text.
    long num = 0;      // valid when kind == Number
    int line = 1;
    double fval = 0.0; // valid when kind == Float
};

// Word-level tokenizer with no keyword/alias resolution — the parser
// decides which words are keywords by matching token text. Parentheses at
// statement boundaries are captured as comments; parentheses inside an
// expression remain ordinary grouping tokens.
class Tokenizer {
public:
    explicit Tokenizer(std::string source) : src_(std::move(source)) {}
    std::vector<Token> tokenize();

private:
    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;

    char peekChar() const;
    char advanceChar();
    bool atEnd() const;
};
