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

`For each` evaluates the list expression once and snapshots its current values when iteration begins. Mutating the original list inside the loop does not change which values remain to be visited, although those mutations still affect the original list after the loop. Lists are otherwise reference values at runtime, so assigning one list variable to another aliases the same mutable collection.

## C scalar type spellings and queries

The first C-capability type surface exposes the ordinary scalar C types through deterministic prose spellings. These spellings are source-level type descriptions and are separate from the legacy inferred `number`/`decimal` value names.

```text
CScalarType ::= "void"
              | "boolean"
              | "character"
              | "signed" "character"
              | "unsigned" "character"
              | "short" "integer"
              | "unsigned" "short" "integer"
              | "integer"
              | "unsigned" "integer"
              | "long" "integer"
              | "unsigned" "long" "integer"
              | "long" "long" "integer"
              | "unsigned" "long" "long" "integer"
              | "float"
              | "decimal"
              | "long" "decimal"
```

They map to C `_Bool`, `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, and `long double`. Plain `character` deliberately remains distinct from both signed and unsigned character types because C defines plain `char` as a distinct type even though its range follows one of them on a target.

Two type-query expressions are currently available:

```text
SizeOfTypeExpr ::= "Size" "of" "type" CScalarType
AlignOfTypeExpr ::= "Alignment" "of" "type" CScalarType
```

Examples:

```text
Say Size of type character. Say Size of type unsigned long long integer. Say Alignment of type long decimal.
```

`Size of type` has C `sizeof(type)` semantics for the supported complete scalar types. `Alignment of type` has C11 `_Alignof(type)` semantics. Both produce a current PlainSpeak whole `number`; a future native `size_t`-equivalent type is still required for complete C object-model parity. Asking either question about `void` is a semantic error because `void` is not an object type.

The exact sizes and alignments of most C scalar types are target properties and are intentionally **not** fixed by PlainSpeak. For example, PlainSpeak does not promise that a `long integer` is eight bytes. The C guarantee that character types occupy one byte is preserved.

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
          | SizeOfTypeExpr
          | AlignOfTypeExpr
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

Precedence, low to high: `or` → `and` → `not` → comparison → additive → multiplicative → power → primary. Unary `minus`, list access, length, C type queries, and function calls bind as primaries. Parentheses inside expressions override precedence.

## Types

Legacy PlainSpeak scalar value types:

- `number` — integer (`long` in the current runtime)
- `decimal` — floating-point (`double` in the current runtime)
- `string` — text

Collection types:

- `list of numbers`
- `list of decimals`
- `list of strings`

The compiler's structural semantic type system additionally represents the C scalar family above plus pointers, arrays, function types, qualifiers, aggregates, enums, C23 bit-precise integers and null pointers as foundations for later syntax. Representation in the compiler is not the same as a user-visible implemented capability; `docs/c-compatibility.md` is authoritative about status.

Every item in a list must have exactly the same scalar type. `List with 1 followed by 2.5 done` is therefore a type error rather than an implicit promotion. Empty lists use the explicit `Empty list of ...` form for the same reason.

`Length of` accepts either a string or a list. `Item at` returns the list's element type. `Append` and `Replace item` require an element of that same type, and list positions must be whole numbers.

Arithmetic between two `number` values produces a `number`; whole-number division truncates toward zero and whole-number modulo uses the corresponding integer remainder. Arithmetic between `number` and `decimal` promotes to `decimal`. `plus` also allows string concatenation with scalar numeric values. Arithmetic and comparison do not operate on whole lists.

## Known gaps

- `Size of type` currently boxes the C `size_t` result into legacy PlainSpeak `number`; native unsigned size types are part of the typed-object tranche.
- Explicit native typed object declarations are not available yet.
- Heap storage used by string concatenation, list snapshots, and lists is released only when the native process exits.
- Lists cannot contain other lists.
- There are no user-defined record/structure declarations yet.
- Procedure parameter and return type declarations are still implicit.

Extend the grammar by following the checklist in `AGENTS.md`; grammar, AST, parser, semantic analysis, code generation, runtime behaviour, and tests must land together.
