#include "alias_table.h"

bool isSetKeyword(const std::string &w) { return w == "set" || w == "let" || w == "make"; }
bool isSayKeyword(const std::string &w) { return w == "say" || w == "print"; }
