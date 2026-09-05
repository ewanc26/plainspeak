#include <cstdlib>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <utility>

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
        std::cerr << "usage: plainspeak <file.eng> [-o output] [--emit-c|--show-generated-c] [--lint] [--define NAME[=VALUE]]\n";
        return 1;
    }

    std::string srcPath = argv[1];
    std::string outPath = "a.out";
    bool emitCOnly = false;
    bool lintOnly = false;
    bool printAstOnly = false;
    std::unordered_map<std::string, long> defines;
    for (int i = 2; i < argc; i++) {
        std::string a = argv[i];
        if (a == "-o" && i + 1 < argc) outPath = argv[++i];
        else if (a == "--emit-c" || a == "--show-generated-c") emitCOnly = true;
        else if (a == "--lint") lintOnly = true;
        else if (a == "--print-ast") printAstOnly = true;
        else if (a == "--define" && i + 1 < argc) {
            std::string definition = argv[++i];
            std::size_t equals = definition.find('=');
            std::string name = equals == std::string::npos ? definition : definition.substr(0, equals);
            std::string valueText = equals == std::string::npos ? "1" : definition.substr(equals + 1);
            if (name.empty()) {
                std::cerr << "error: --define needs a macro name\n";
                return 1;
            }
            for (std::size_t j = 0; j < name.size(); ++j) {
                const unsigned char c = static_cast<unsigned char>(name[j]);
                const bool valid = (j == 0) ? (std::isalpha(c) || c == '_') :
                                             (std::isalnum(c) || c == '_');
                if (!valid) {
                    std::cerr << "error: --define needs a C identifier name\n";
                    return 1;
                }
            }
            char *end = nullptr;
            long value = std::strtol(valueText.c_str(), &end, 10);
            if (!end || *end != '\0') {
                std::cerr << "error: --define value must be a whole number\n";
                return 1;
            }
            defines[name] = value;
        } else if (a == "--define") {
            std::cerr << "error: --define needs NAME or NAME=VALUE\n";
            return 1;
        }
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

    Sema sema(std::move(defines));
    AnalysisResult analysis = sema.analyze(program);
    bool hasErrors = false;
    for (const auto &d : analysis.diagnostics) {
        std::cerr << (d.severity == DiagSeverity::Warning ? "warning" : "error")
                  << "[E" << std::setfill('0') << std::setw(4) << d.code << "]: " << d.message << "\n";
        if (d.severity == DiagSeverity::Error) hasErrors = true;
    }
    if (hasErrors) return 1;

    if (printAstOnly) {
        std::cout << printAST(program);
        return 0;
    }

    if (lintOnly) {
        std::cout << "No lint issues found.\n";
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
    // query. C99 programs remain valid C11 programs. CI and users targeting
    // C23-only facilities can select a capable compiler without changing the
    // PlainSpeak source via PLAINSPEAK_CC.
    const char *configuredCompiler = std::getenv("PLAINSPEAK_CC");
    std::string compiler = configuredCompiler && *configuredCompiler ? configuredCompiler : "cc";
    std::string cmd = compiler + " -std=c11 -O2 -I" PLAINSPEAK_RUNTIME_DIR
                       " \"" + tmpC + "\" \"" PLAINSPEAK_RUNTIME_C "\" -lm";
    for (const auto &library : analysis.cLibraries) cmd += " -l" + library;
    cmd += " -o \"" + outPath + "\"";
    int rc = std::system(cmd.c_str());
    if (rc != 0) {
        std::cerr << "error: generated C failed to compile (this is a plainspeak bug, "
                     "not a mistake in your program) — see " << tmpC << "\n";
        return 1;
    }
    return 0;
}
