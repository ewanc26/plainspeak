# Grammar (v1)

Source of truth for syntax — the parser (`src/parser/parser.cpp`) must accept exactly what's described here, no more, no less. Every sentence pattern below has end-to-end coverage in `tests/golden/`.

## Source is prose

PlainSpeak source is written as prose paragraphs, not as indented code. A normal program places sentences next to each other with spaces:

```text
Set total to 0. (Keep a running total.) Repeat 3: Add 2 to total. End repeat. Say total.
```

Physical newlines and indentation are ordinary whitespace only. They never open, close, or nest a block, so editors may wrap a paragraph without changing the program. `.` ends a sentence, `:` opens a compound sentence, and explicit `End <block>.` phrases close blocks.

Standalone parentheticals are comments:

```text
Set total to 0. (This explanation is ignored by the program.) Add 1 to total.
```

A `(` encountered where a statement can begin starts a comment and preserves the text inside it. Parentheses encountered inside an expression remain expression grouping, so `Say (2 plus 3) times 4.` is not a comment. `#` comments and the old `Comment ... .` sentence form are not part of the grammar.

## Lexical notes

- Identifiers and keywords are case-insensitive.
- Strings are double-quoted, with `\"`, `\\`, and `\n` escapes.
- Decimal numbers such as `3.14` are single `Float` tokens.
- Commas are optional prose punctuation and have no syntactic meaning.
- Parenthetical comments may contain ordinary punctuation and nested parentheses.
- Newlines are equivalent to spaces.

## Synonyms

Resolved by exact, case-insensitive lookup — never fuzzy matching:

| Canonical | Accepted words |
|---|---|
| Set | `set`, `let`, `make` |
| Say | `say`, `print` |

## Statements

```text
Stmt ::= SayStmt | SetStmt | AddStmt | SubStmt | ReadStmt | ReadFloatStmt
       | AppendStmt | ReplaceItemStmt | RemoveItemStmt | CommentStmt
       | RepeatStmt | IfStmt | WhileStmt | ForEachStmt
       | CallStmt | ProcedureStmt | ReturnStmt

SayStmt ::= ("Say" | "Print") Expr "."
SetStmt ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
AddStmt ::= "Add" Expr "to" IDENT "."
SubStmt ::= "Subtract" Expr "from" IDENT "."
ReadStmt ::= "Read" IDENT "."
ReadFloatStmt ::= "ReadFloat" IDENT "."
AppendStmt ::= "Append" Expr "to" IDENT "."
ReplaceItemStmt ::= "Replace" "item" "at" Expr "in" IDENT "with" Expr "."
RemoveItemStmt ::= "Remove" "item" "at" Expr "from" IDENT "."
CommentStmt ::= "(" COMMENT_TEXT ")"
RepeatStmt ::= "Repeat" Expr ":" Stmt* "End" "repeat" "."
IfStmt ::= "If" Expr "then" ":" Stmt* ("Else" ":" Stmt*)? "End" "if" "."
WhileStmt ::= "While" Expr ":" Stmt* "End" "while" "."
ForEachStmt ::= "For" "each" IDENT "in" Expr ":" Stmt* "End" "for" "."
CallStmt ::= "Call" IDENT ("with" Expr ("," Expr)*)? "done" "."
ProcedureStmt ::= "Procedure" IDENT ("takes" IDENT ("," IDENT)*)? ":" Stmt* "End" "procedure" "."
ReturnStmt ::= "Return" Expr "."
```

Examples are intentionally formatted as paragraphs:

```text
Say "Hello, world!". Set total to 0. Add 1 to total. Subtract 1 from total.
```

```text
If total is greater than 5 then: Say "big". Else: Say "small". End if. While total is less than 10: Add 1 to total. End while.
```

```text
Procedure greet takes name: Say name. End procedure. Call greet with "world" done.
```

## Lists

Lists are mutable, homogeneous collections of numbers, decimals, or strings. Nested lists are rejected by semantic analysis. Positions are **one-based**, matching how a person would normally say “the first item”.

A non-empty list states its items in order:

```text
Set primes to List with 2 followed by 3 followed by 5 done.
```

An empty list states its element type so the compiler does not need to guess:

```text
Set names to Empty list of strings. Append "Ada" to names.
```

Reading and mutation use prose operations:

```text
Say Item at 2 in primes. Replace item at 2 in primes with 11. Remove item at 1 from primes. Append 7 to primes.
```

Iteration is a block sentence and introduces a scoped item name:

```text
For each prime in primes: Say prime. End for.
```

`For each` evaluates the list expression once and remembers its length when iteration begins. The loop then reads the current value at each one-based position. Lists are reference values at runtime, so assigning one list variable to another aliases the same mutable collection.

## Expressions

```text
Expr ::= OrExpr
OrExpr ::= AndExpr ("or" AndExpr)*
AndExpr ::= NotExpr ("and" NotExpr)*
NotExpr ::= "not" NotExpr | Comparison
Comparison ::= Additive ("is" Comparator Additive)?

Comparator ::= "greater" "than"
             | "less" "than"
             | "equal" "to"
             | "not" "equal" "to"
             | "greater" "than" "or" "equal" "to"
             | "less" "than" "or" "equal" "to"

Additive ::= Multiplicative (("plus" | "minus") Multiplicative)*
Multiplicative ::= Power (("times" | "divided by" | "mod" | "modulo") Power)*
Power ::= Primary ("to" "the" "power" "of" Primary)*

Primary ::= NUMBER | FLOAT | STRING | IDENT | "true" | "false"
          | "minus" Primary
          | "(" Expr ")"
          | "Length" "of" Primary
          | ListExpr
          | EmptyListExpr
          | ItemExpr
          | "square" "root" "of" Primary
          | "absolute" "value" "of" Primary
          | ("sine" | "cosine" | "tangent" | "sqrt" | "log" | "abs" | "floor" | "ceil") "of" Primary
          | "Call" IDENT ("with" Expr ("," Expr)*)? "done"

ListExpr ::= "List" "with" Expr ("followed" "by" Expr)* "done"
EmptyListExpr ::= "Empty" "list" "of" ("numbers" | "decimals" | "strings")
ItemExpr ::= "Item" "at" Expr "in" Primary
```

Precedence, low to high: `or` → `and` → `not` → comparison → additive → multiplicative → power → primary. Unary `minus`, list access, length, and function calls bind as primaries. Parentheses inside expressions override precedence.

## Types

Scalar types:

- `number` — integer (`long` in generated C)
- `decimal` — floating-point (`double` in generated C)
- `string` — text

Collection types:

- `list of numbers`
- `list of decimals`
- `list of strings`

Every item in a list must have exactly the same scalar type. `List with 1 followed by 2.5 done` is therefore a type error rather than an implicit promotion. Empty lists use the explicit `Empty list of ...` form for the same reason.

`Length of` accepts either a string or a list. `Item at` returns the list's element type. `Append` and `Replace item` require an element of that same type, and list positions must be whole numbers.

Arithmetic between `number` and `decimal` promotes to `decimal`. `plus` also allows string concatenation with scalar numeric values. Arithmetic and comparison do not operate on whole lists.

## Known gaps

- Heap storage used by string concatenation and lists is released only when the native process exits.
- Lists cannot contain other lists.
- There are no user-defined record/structure types.
- Procedure parameter and return type declarations are still implicit.

Extend the grammar by following the checklist in `AGENTS.md`; grammar, AST, parser, semantic analysis, code generation, runtime behaviour, and tests must land together.
