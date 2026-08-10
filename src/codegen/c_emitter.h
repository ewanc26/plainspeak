#pragma once
#include <string>
#include <vector>

#include "../ast/ast.h"

// AST -> C99 source, textual. One pass, no separate "codegen IR" — see
// AGENTS.md §7 for the mangling/runtime-call conventions this follows.
std::string emitProgram(const std::vector<Stmt *> &program);
