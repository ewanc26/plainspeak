#include "alias_table.h"

bool isSetKeyword(const std::string &w) { return w == "set" || w == "let" || w == "make" || w == "assign" || w == "put"; }
bool isSayKeyword(const std::string &w) { return w == "say" || w == "print" || w == "show" || w == "display" || w == "write" || w == "output"; }
bool isDeclareKeyword(const std::string &w) { return w == "declare" || w == "create"; }
bool isAddKeyword(const std::string &w) { return w == "add" || w == "sum"; }
bool isSubtractKeyword(const std::string &w) { return w == "subtract" || w == "deduct" || w == "take"; }
bool isIncreaseKeyword(const std::string &w) { return w == "increase" || w == "raise" || w == "grow"; }
bool isDecreaseKeyword(const std::string &w) { return w == "decrease" || w == "reduce" || w == "lower"; }
bool isAppendKeyword(const std::string &w) { return w == "append" || w == "push"; }
bool isReturnKeyword(const std::string &w) { return w == "return" || w == "yield"; }
bool isUnlessKeyword(const std::string &w) { return w == "unless"; }
bool isUntilKeyword(const std::string &w) { return w == "until"; }