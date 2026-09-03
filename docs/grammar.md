# Grammar (v1)

Source of truth for syntax — the parser (`src/parser/parser.cpp`) must accept exactly what's described here, no more, no less. Every sentence pattern below has end-to-end coverage in `tests/golden/`.

## Source is prose

PlainSpeak source is written as prose paragraphs, not as indented code. A normal program places sentences next to each other with spaces:

```text
Set total to 0. (Keep a running total.) Repeat 3: Add 2 to total. End repeat. Say total.
```

Physical newlines and indentation are ordinary whitespace only. They never open, close, or nest a block, so editors may wrap a paragraph without changing the program. `.` ends a sentence, `:` opens a compound sentence, and explicit `End <block>.` phrases close blocks.

Standalone parentheticals are comments. A `(` encountered where a statement can begin starts a comment and preserves the text inside it. Parentheses inside an expression remain expression grouping. `#` comments and the old `Comment ... .` sentence form are not part of the grammar.

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
Stmt ::= SayStmt | SetStmt | DeclareStmt | StoreThroughStmt
       | AddStmt | SubStmt | ReadStmt | ReadFloatStmt
       | AppendStmt | ReplaceItemStmt | RemoveItemStmt | CommentStmt
       | RepeatStmt | IfStmt | WhileStmt | ForEachStmt
       | CallStmt | ProcedureStmt | ReturnStmt

SayStmt ::= ("Say" | "Print") Expr "."
SetStmt ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
DeclareStmt ::= "Declare" IDENT "as" CType ("with" "value" Expr)? "."
StoreThroughStmt ::= ("Set" | "Let" | "Make") "value" "at" Expr "to" Expr "."
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

`Set` has two related roles. If its name does not exist in the current visible scopes, it creates the existing inferred boxed PlainSpeak variable. If that name already denotes a variable, `Set` assigns a new value to it instead. Explicit C-compatible objects are introduced only with `Declare`.

Examples are intentionally formatted as paragraphs:

```text
Say "Hello, world!". Set total to 0. Add 1 to total. Set total to 9. Say total.
```

```text
If total is greater than 5 then: Say "big". Else: Say "small". End if. While total is less than 10: Add 1 to total. End while.
```

## Lists

Lists are mutable, homogeneous collections of numbers, decimals, or strings. Nested lists and native pointers are not list element types in the current language. Positions are **one-based**.

```text
Set primes to List with 2 followed by 3 followed by 5 done. Say Item at 2 in primes. Replace item at 2 in primes with 11. Remove item at 1 from primes. Append 7 to primes.
```

```text
Set names to Empty list of strings. Append "Ada" to names. For each name in names: Say name. End for.
```

`For each` evaluates the list expression once and snapshots its current values when iteration begins. Mutating the original list inside the loop does not change which values remain to be visited. Lists are otherwise reference values at runtime.

## C type spellings

PlainSpeak exposes the ordinary C scalar family through deterministic prose spellings, plus recursive object-pointer types:

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

CType ::= CScalarType | "pointer" "to" CType
```

The scalar spellings map to C `_Bool`, `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, and `long double`. Plain `character` remains distinct from both signed and unsigned character types, matching C.

`pointer to` is recursive:

```text
Declare p as pointer to integer. Declare pp as pointer to pointer to integer.
```

A `void` type may be the pointee of a pointer, but a standalone object cannot be declared as `void`.

## Native objects and pointers

`Declare` creates an object with actual C storage and layout rather than a boxed `PsValue`:

```text
Declare count as integer with value 41. Declare fraction as float with value 2.5.
```

A direct top-level declaration has translation-unit scope and static storage duration in the generated program. Its optional PlainSpeak initializer executes during program startup in source order, so it may use ordinary PlainSpeak expressions. A declaration inside a procedure or block is emitted as an automatic C object at that point. A top-level declaration without an initializer receives C's zero initialization; an automatic declaration without an initializer has C's ordinary indeterminate initial value and must not be read before a value is assigned.

The address and indirection operations are prose equivalents of C `&` and unary `*`:

```text
Declare count as integer with value 41. Declare where as pointer to integer with value Address of count. Set value at where to 42. Say Value at where.
```

`Address of IDENT` is valid only for an explicit native object. It deliberately does not expose the address of a legacy boxed `PsValue`, because that box is an implementation detail rather than the C object represented by the PlainSpeak value.

`Value at pointer` dereferences a non-void object pointer. `Set value at pointer to value.` stores through that pointer. Pointer-to-pointer indirection composes naturally:

```text
Declare x as integer with value 7. Declare p as pointer to integer with value Address of x. Declare pp as pointer to pointer to integer with value Address of p. Say Value at Value at pp.
```

Native arithmetic scalar declarations accept arithmetic initializers and assignments using C assignment conversion at the generated-C boundary. Compatible object pointers may be assigned directly; object-pointer/`void *` compatibility is recognised. Full integer promotions and the complete usual-arithmetic-conversion model remain separate conformance work.

Pointer arithmetic, relational/equality pointer comparison, pointer truthiness, function pointers, pointer-valued legacy procedure parameters/returns, qualifiers, arrays, and allocation are **not** enabled by this tranche.

## Size and alignment queries

```text
SizeOfTypeExpr ::= "Size" "of" "type" CType
SizeOfExpr ::= "Size" "of" Primary
AlignOfTypeExpr ::= "Alignment" "of" "type" CType
```

Examples:

```text
Say Size of type character. Say Size of type pointer to integer. Say Alignment of type long decimal. Declare x as integer with value 1. Say Size of x.
```

`Size of type` lowers to C `sizeof(type)`. `Alignment of type` lowers to C11 `_Alignof(type)`. `Size of` an expression uses the semantic type of the operand and does not evaluate that operand, matching the unevaluated nature of ordinary non-VLA C `sizeof` for the currently supported native object types.

Results are currently boxed back into PlainSpeak `number`, so native `size_t` is still pending. Exact scalar and pointer sizes/alignments remain target properties; PlainSpeak does not impose LP64 or another data model.

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
          | "Address" "of" IDENT
          | "Value" "at" Primary
          | "Length" "of" Primary
          | SizeOfTypeExpr
          | SizeOfExpr
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

Precedence, low to high: `or` → `and` → `not` → comparison → additive → multiplicative → power → primary. Addressing, indirection, list access, length, size/alignment queries, and calls bind as primaries. Parentheses override precedence.

## Types and representation boundary

Legacy PlainSpeak values remain:

- `number` — boxed signed C `long`
- `decimal` — boxed C `double`
- `string` — runtime text
- homogeneous mutable lists of those values

Explicit `Declare` objects instead use native C storage for the `CType` written in source. These two representations are intentionally distinct. Reading a native arithmetic object in a legacy expression boxes its current value; assigning a legacy numeric expression into a native arithmetic object converts it back to that object's C type.

The compiler's structural semantic type system also represents arrays, functions, qualifiers, aggregates, enums, C23 bit-precise integers and null pointers as foundations for later syntax. Representation in the compiler is not itself a claim of supported source capability; `docs/c-compatibility.md` is authoritative about status.

## Known gaps

- `sizeof`/alignment results are boxed into legacy `number`; a first-class unsigned `size_t`-equivalent remains pending.
- Pointer arithmetic/comparison and pointer conditions are pending.
- Arrays/VLAs, structs/unions/bit-fields, enums, qualifiers, atomics, function pointers, storage/linkage specifiers, allocation and C23 pointer additions remain pending.
- Procedure parameter and return type declarations remain implicit and cannot transport native pointers yet.
- Heap storage used by string concatenation, list snapshots, and lists is released only at process exit.
- Lists cannot contain lists or native pointers.

Extend the grammar by following the checklist in `AGENTS.md`; grammar, AST, parser, semantic analysis, code generation, runtime behaviour, documentation, and tests must land together.
