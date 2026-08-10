#pragma once
#include <string>
#include <vector>

enum class TokKind { Ident, Number, Float, String, Dot, Colon, LParen, RParen, Eof };

struct Token {
    TokKind kind;
    std::string text; // for Ident/String: the value. Idents are lowercased
                       // for keyword matching; original case is not needed
                       // since identifiers are case-insensitive in v0.
    long num = 0;      // valid when kind == Number
    int line = 1;
    double fval = 0.0; // valid when kind == Float
};

// Word-level tokenizer with no keyword/alias resolution — the parser
// decides which words are keywords by matching token text. This keeps the
// alias table (docs/grammar.md §synonyms) in one place: parser.cpp's
// `matchWord()` helper.
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
