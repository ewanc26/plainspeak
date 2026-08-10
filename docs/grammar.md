# Grammar (v0)

Source of truth for syntax — the parser (`src/parser/parser.cpp`) must
accept exactly what's described here, no more, no less. Every rule below
has a corresponding golden test in `tests/golden/`.

## Lexical notes

- Statements end with `.`; blocks are opened with `:` and closed with a
  two-word `End <block>.` sentence.
- Commas are ignored by the lexer (purely readability, no syntactic
  meaning yet).
- Identifiers and keywords are case-insensitive.
- Strings are double-quoted, with `\"`, `\\`, `\n` escapes.

## Synonyms (alias table)

Resolved in `parser.cpp` (`isSetKeyword`, `isSayKeyword`) — a flat,
case-insensitive exact match, never fuzzy (AGENTS.md §4.2):

| Canonical | Accepted words   |
|-----------|-------------------|
| Set       | `set`, `let`, `make` |
| Say       | `say`, `print`    |

## Statements

```
Stmt        ::= SayStmt | SetStmt | AddStmt | RepeatStmt | IfStmt | WhileStmt

SayStmt     ::= ("Say" | "Print") Expr "."
                e.g. Say "Hello, world!".
                e.g. Say total.

SetStmt     ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
                e.g. Set total to 0.

AddStmt     ::= "Add" Expr "to" IDENT "."
                e.g. Add 1 to total.

RepeatStmt  ::= "Repeat" Expr "times" ":" Stmt* "End" "repeat" "."
                e.g.
                Repeat 5 times:
                    Add 1 to total.
                End repeat.

IfStmt      ::= "If" Expr "then" ":" Stmt* ("Else" ":" Stmt*)? "End" "if" "."
                e.g.
                If total is greater than 5 then:
                    Say "big".
                Else:
                    Say "small".
                End if.

WhileStmt   ::= "While" Expr ":" Stmt* "End" "while" "."
                e.g.
                While x is less than 3:
                    Say x.
                    Set x to x plus 1.
                End while.
```

## Expressions

```
Expr        ::= Additive ( "is" Comparator Additive )?
Comparator  ::= "greater" "than" | "less" "than" | "equal" "to"

Additive    ::= Primary ( "plus" Primary )*

Primary     ::= NUMBER | STRING | IDENT
```

Precedence, low to high: comparison → additive → primary. Comparisons do
not chain (`a is greater than b is greater than c` is a parse error — the
second `is` has nothing valid on its left).

## Known gaps in v0 (deliberately out of scope for the scaffold)

- No user-defined procedures.
- No static type checking — type mismatches (e.g. `Add "x" to 5`) are
  caught at runtime by `plainspeak_runtime.c`, not by a sema pass. A real
  sema pass belongs in `src/sema/` per AGENTS.md's pipeline and should
  move these checks to compile time.
- String concatenation results are heap-allocated and never freed.

Extend the grammar by following the 8-step checklist in AGENTS.md §5 —
don't add a pattern here without also adding its test.
