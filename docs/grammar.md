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
- `#` starts a single-line comment that runs to the end of the line.

## Synonyms (alias table)

Resolved in `parser.cpp` (`isSetKeyword`, `isSayKeyword`) — a flat,
case-insensitive exact match, never fuzzy (AGENTS.md §4.2):

| Canonical | Accepted words   |
|-----------|-------------------|
| Set       | `set`, `let`, `make` |
| Say       | `say`, `print`    |

## Statements

```
Stmt        ::= SayStmt | SetStmt | AddStmt | RepeatStmt | IfStmt | WhileStmt | CallStmt | ProcedureStmt | ReturnStmt

SayStmt     ::= ("Say" | "Print") Expr "."
                e.g. Say "Hello, world!".
                e.g. Say total.

SetStmt     ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
                e.g. Set total to 0.

AddStmt     ::= "Add" Expr "to" IDENT "."
                e.g. Add 1 to total.

SubStmt     ::= "Subtract" Expr "from" IDENT "."
                e.g. Subtract 1 from total.

ReadStmt    ::= "Read" IDENT "."
                e.g. Read x.

RepeatStmt  ::= "Repeat" Expr ":" Stmt* "End" "repeat" "."
                e.g.
                Repeat 5:
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

CallStmt    ::= "Call" IDENT "with" Expr ("," Expr)* "."
                e.g. Call greet with "world".
                e.g. Call add with 1, 2.

ProcedureStmt ::= "Procedure" IDENT "takes" IDENT ("," IDENT)* ":" Stmt* "End" "procedure" "."
                e.g.
                Procedure greet takes name:
                    Say name.
                End procedure.

ReturnStmt  ::= "Return" Expr "."
                e.g. Return x plus 1.
```

## Expressions

```
Expr           ::= OrExpr
OrExpr         ::= AndExpr ( "or" AndExpr )*
AndExpr        ::= NotExpr ( "and" NotExpr )*
NotExpr        ::= "not" NotExpr | Comparison
Comparison     ::= Additive ( "is" Comparator Additive )?

Comparator     ::= "greater" "than" | "less" "than" | "equal" "to"

Additive       ::= Multiplicative ( ("plus" | "minus") Multiplicative )*

Multiplicative ::= Primary ( ("times" | "divided by") Primary )*

Primary        ::= NUMBER | STRING | IDENT | "true" | "false"
```

Precedence, low to high: `or` → `and` → `not` → comparison → additive → multiplicative.
`not` binds tighter than `and`, which binds tighter than `or`.

## Known gaps in v0 (deliberately out of scope for the scaffold)

- String concatenation results are heap-allocated and never freed.

Extend the grammar by following the 8-step checklist in AGENTS.md §5 —
don't add a pattern here without also adding its test.
