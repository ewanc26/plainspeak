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
            tokens.push_back({TokKind::Dot, ".", 0, line});
            continue;
        }

        if (c == ':') {
            int line = line_;
            advanceChar();
            tokens.push_back({TokKind::Colon, ":", 0, line});
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
            tokens.push_back({TokKind::String, value, 0, line});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            int line = line_;
            std::string digits;
            while (!atEnd() && std::isdigit(static_cast<unsigned char>(peekChar())))
                digits += advanceChar();
            tokens.push_back({TokKind::Number, digits, std::stol(digits), line});
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
            tokens.push_back({TokKind::Ident, lowered, 0, line});
            continue;
        }

        throw std::runtime_error("unexpected character '" + std::string(1, c) +
                                  "' on line " + std::to_string(line_));
    }

    tokens.push_back({TokKind::Eof, "", 0, line_});
    return tokens;
}
