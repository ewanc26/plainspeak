#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/ast.h"

std::string emitProgram(const std::vector<Stmt *> &program,
                        const std::unordered_map<int, std::string> *sourceLines = nullptr);
