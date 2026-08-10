#include "mangling.h"
#include <unordered_set>

namespace {

const std::unordered_set<std::string> c_keywords = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while", "inline", "restrict", "_Bool", "_Complex", "_Imaginary",
    "_Alignas", "_Alignof", "_Atomic", "_Generic", "_Noreturn",
    "_Static_assert", "_Thread_local", "alignas", "alignof", "bool",
    "constexpr", "nullptr", "static_assert", "thread_local", "_BitInt",
    "_Decimal128", "_Decimal32", "_Decimal64", "typeof", "typeof_unqual",
    "true", "false", "asm", "fortran"
};

const std::unordered_set<std::string> runtime_symbols = {
    "ps_int", "ps_str", "ps_add", "ps_gt", "ps_lt", "ps_eq", "ps_as_int",
    "ps_truthy", "ps_say", "PsValue", "PS_INT", "PS_STRING"
};

}

std::string mangle(const std::string &name) {
    std::string candidate = "ps_" + name;
    if (c_keywords.count(candidate) || runtime_symbols.count(candidate)) {
        return "_ps_" + name;
    }
    return candidate;
}
