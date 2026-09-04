#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

#include "../ast/ast.h"
#include "../ast/ast_printer.h"
#include "../codegen/c_emitter.h"
#include "../lexer/tokenizer.h"
#include "../parser/parser.h"
#include "../sema/sema.h"

#ifndef PLAINSPEAK_RUNTIME_C
#error "PLAINSPEAK_RUNTIME_C must be defined by the build system"
#endif
#ifndef PLAINSPEAK_RUNTIME_DIR
#error "PLAINSPEAK_RUNTIME_DIR must be defined by the build system"
#endif

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: plainspeak <file.eng> [-o output] [--emit-c]\n";
        return 1;
    }

    std::string srcPath = argv[1];
    std::string outPath = "a.out";
    bool emitCOnly = false;
    bool printAstOnly = false;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) outPath = argv[++i];
        else if (a == "--emit-c") emitCOnly = true;
        else if (a == "--print-ast") printAstOnly = true;
    }

    std::ifstream in(srcPath);
    if (!in) {
        std::cerr << "error: cannot open \"" << srcPath << "\"\n";
        return 1;
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string source = ss.str();

    std::unordered_map<int, std::string> sourceLines;
    {
        std::istringstream lineStream(source);
        std::string line;
        int lineNum = 1;
        while (std::getline(lineStream, line)) sourceLines[lineNum++] = line;
    }

    std::vector<Token> tokens;
    try {
        Tokenizer tokenizer(source);
        tokens = tokenizer.tokenize();
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    Arena arena;
    std::vector<Stmt *> program;
    try {
        Parser parser(tokens, arena);
        program = parser.parseProgram();
    } catch (const ParseError &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }

    Sema sema;
    AnalysisResult analysis = sema.analyze(program);
    for (const auto &d : analysis.diagnostics) {
        std::cerr << "error[E" << std::setfill('0') << std::setw(4) << d.code << "]: " << d.message << "\n";
    }
    if (!analysis.diagnostics.empty()) return 1;

    if (printAstOnly) {
        std::cout << printAST(program);
        return 0;
    }

    std::string cSource = emitProgram(program, analysis, &sourceLines);

    if (emitCOnly) {
        std::cout << cSource;
        return 0;
    }

    std::string tmpC = outPath + ".gen.c";
    {
        std::ofstream out(tmpC);
        out << cSource;
    }

    // C11 is the first backend dialect needed beyond the C99 baseline: it
    // supplies the standard _Alignof operator used by PlainSpeak's alignment
    // query. C99 programs remain valid C11 programs.
    std::string cmd = "cc -std=c11 -O2 -I" PLAINSPEAK_RUNTIME_DIR
                       " \"" + tmpC + "\" \"" PLAINSPEAK_RUNTIME_C "\" -lm -o \"" + outPath + "\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "error: generated C failed to compile (this is a plainspeak bug, "
                     "not a mistake in your program) — see " << tmpC << "\n";
        return 1;
    }
    return 0;
}
