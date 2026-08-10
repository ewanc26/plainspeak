#include "tokenizer.h"
#include <cctype>
#include <stdexcept>

char Tokenizer::peekChar() const {
    return pos_ < src_.size() ? src_[pos_] : '\0';
}

char Tokenizer::advanceChar() {
    char c = src_[pos_++];
    if (c == '\n') line_++;
    return c;
}

bool Tokenizer::atEnd() const { return pos_ >= src_.size(); }

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;

    while (!atEnd()) {
        char c = peekChar();

        if (c == '#') {
            while (!atEnd() && peekChar() != '\n') advanceChar();
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c))) {
            advanceChar();
            continue;
        }

        if (c == ',') { // commas separate clauses; no syntactic meaning yet
            advanceChar();
            continue;
        }

        if (c == '.') {
            int line = line_;
            advanceChar();
            Token t;
            t.kind = TokKind::Dot;
            t.text = ".";
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        if (c == '(') {
            int line = line_;
            advanceChar();
            Token t;
            t.kind = TokKind::LParen;
            t.text = "(";
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        if (c == ')') {
            int line = line_;
            advanceChar();
            Token t;
            t.kind = TokKind::RParen;
            t.text = ")";
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        if (c == ':') {
            int line = line_;
            advanceChar();
            Token t;
            t.kind = TokKind::Colon;
            t.text = ":";
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        if (c == '"') {
            int line = line_;
            advanceChar(); // opening quote
            std::string value;
            while (!atEnd() && peekChar() != '"') {
                char ch = advanceChar();
                if (ch == '\\' && !atEnd()) {
                    char esc = advanceChar();
                    value += (esc == 'n') ? '\n' : esc;
                } else {
                    value += ch;
                }
            }
            if (atEnd()) {
                throw std::runtime_error("unterminated string literal starting on line " +
                                          std::to_string(line));
            }
            advanceChar(); // closing quote
            Token t;
            t.kind = TokKind::String;
            t.text = value;
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            int line = line_;
            std::string digits;
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peekChar())))
                digits += advanceChar();
            if (peekChar() == '.' && pos_ + 1 < src_.size() &&
                std::isdigit(static_cast<unsigned char>(src_[pos_ + 1]))) {
                digits += advanceChar(); // consume '.'
                while (!atEnd() && std::isdigit(static_cast<unsigned char>(peekChar())))
                    digits += advanceChar();
                Token t;
                t.kind = TokKind::Float;
                t.text = digits;
                t.line = line;
                t.fval = std::stod(digits);
                tokens.push_back(t);
            } else {
                Token t;
                t.kind = TokKind::Number;
                t.text = digits;
                t.num = std::stol(digits);
                t.line = line;
                tokens.push_back(t);
            }
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            int line = line_;
            std::string word;
            while (!atEnd() &&
                   (std::isalnum(static_cast<unsigned char>(peekChar())) || peekChar() == '_'))
                word += advanceChar();
            std::string lowered;
            for (char ch : word) lowered += static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            Token t;
            t.kind = TokKind::Ident;
            t.text = lowered;
            t.line = line;
            tokens.push_back(t);
            continue;
        }

        throw std::runtime_error("unexpected character '" + std::string(1, c) +
                                  "' on line " + std::to_string(line_));
    }

    tokens.push_back({TokKind::Eof, "", 0, line_});
    return tokens;
}
