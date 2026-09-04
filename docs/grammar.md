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
Stmt ::= SayStmt | SetStmt | DeclareStmt | StoreThroughStmt | StoreElementStmt
       | AddStmt | SubStmt | ReadStmt | ReadFloatStmt
       | AppendStmt | ReplaceItemStmt | RemoveItemStmt | CommentStmt
       | RepeatStmt | IfStmt | WhileStmt | ForEachStmt
       | CallStmt | ProcedureStmt | ReturnStmt | StructureStmt

SayStmt ::= ("Say" | "Print") Expr "."
SetStmt ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
DeclareStmt ::= "Declare" IDENT "as" CType ("with" "value" Expr)? "."
StoreThroughStmt ::= ("Set" | "Let" | "Make") "value" "at" Expr "to" Expr "."
StoreElementStmt ::= ("Set" | "Let" | "Make") "element" "at" Expr "in" Expr "to" Expr "."
StoreMemberStmt ::= ("Set" | "Let" | "Make") "member" IDENT "of" Expr "to" Expr "."
StructureField ::= "Field" IDENT "as" CType "."
StructureStmt ::= "Structure" IDENT ":" StructureField+ "End" "structure" "."
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
ProcedureParam ::= IDENT ("as" CType)?
ProcedureStmt ::= "Procedure" IDENT ("takes" ProcedureParam ("," ProcedureParam)*)? ("returns" CType)? ":" Stmt* "End" "procedure" "."
ReturnStmt ::= "Return" Expr? "."
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

PlainSpeak exposes the ordinary C scalar family through deterministic prose spellings, plus recursive object-pointer, fixed-array, and tagged structure types:

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

CType ::= CScalarType
        | "pointer" "to" CType
        | "array" "of" CType "with" "length" NUMBER
        | "structure" IDENT
```

The scalar spellings map to C `_Bool`, `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, and `long double`. Plain `character` remains distinct from both signed and unsigned character types, matching C.

`pointer to` is recursive:

```text
Declare p as pointer to integer. Declare pp as pointer to pointer to integer.
```

A `void` type may be the pointee of a pointer, but a standalone object or array element cannot be `void`. Fixed array bounds are positive whole-number literals. Arrays and pointers may nest recursively, so `pointer to array of integer with length 4` and `array of pointer to integer with length 4` are distinct types and lower to distinct C declarators.

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

## Native fixed arrays and pointer arithmetic

Fixed native arrays use real C array storage:

```text
Declare values as array of integer with length 4. Set element at 0 in values to 10. Set element at 1 in values to 20. Say Element at 1 in values.
```

Native `Element at` is deliberately **zero-based**, matching C subscripting. It is separate from one-based mutable-list `Item at`. `Element at index in base` accepts a fixed native array or an object pointer; `Set element at index in base to value.` writes through that subscript.

An array expression keeps its full array type when used by `Size of` or `Address of`. In ordinary value contexts it decays to a pointer to its first element, so this is valid:

```text
Declare values as array of integer with length 4. Declare p as pointer to integer with value values. Declare q as pointer to integer with value p plus 2. Say Value at q. Say q minus p.
```

`pointer plus integer`, `integer plus pointer`, and `pointer minus integer` use C element-scaled pointer arithmetic for complete object pointers. Subtracting two pointers to the same element type produces the current PlainSpeak whole-number result. Equality comparison accepts compatible object pointers (including the existing object-pointer/`void *` compatibility); relational comparison requires the same complete element type. `Add` and `Subtract` on an explicitly declared object pointer lower to C `+=` and `-=` with an integer offset.

Whole-array assignment and whole-array initializers are intentionally not invented. Declare an array, then set elements individually. Variable-length arrays, pointer truthiness, null pointers, function pointers, qualifiers and allocated storage remain later work.

## Native structures and members

A tagged structure definition creates real C aggregate layout:

```text
Structure point: Field x as integer. Field y as integer. End structure. Declare p as structure point. Set member x of p to 10. Say Member x of p.
```

Structure tags form their own semantic namespace and are pre-registered as incomplete before field checking. This permits self-referential and forward pointers such as `pointer to structure node`, while a recursive or forward structure used **by value** must already be complete at that definition point.

`Member name of base` reads a field from either a structure object or a pointer to one. `Set member name of base to value.` writes a non-array field after normal native assignment checking. The compiler chooses C `.` or `->` from the semantic base type; the prose syntax does not expose that punctuation distinction.

Fixed arrays and already-complete structures may be fields. Array fields can be reached with native `Element at`, including nested expressions. Whole-array member assignment is still intentionally rejected.

Native structures can be copied by assignment and transported by value through typed Procedures when their tags match, using the generated C ABI. Structure initializers/literals, designated initializers, anonymous members, flexible array members, unions and bit-fields remain separate work.

## Procedures and typed signatures

Legacy procedures remain source-compatible:

```text
Procedure greet takes name: Say name. End procedure. Call greet with "world" done.
```

A typed procedure gives every parameter a C type and explicitly states its return type:

```text
Procedure add takes left as integer, right as integer returns integer: Return left plus right. End procedure. Say Call add with 4, 5 done.
```

A procedure may also have no parameters while still being typed, for example `Procedure answer returns integer: Return 42. End procedure.` Typed and untyped parameters cannot be mixed, and a typed parameter list requires an explicit `returns` clause. Use `returns void` when no value is returned.

Typed parameters are real native C objects inside the procedure. Scalar and pointer parameter types therefore preserve their C representation. A fixed array parameter is adjusted to a pointer to its first element, matching C function-parameter adjustment:

```text
Procedure first takes values as array of integer with length 4 returns integer: Return Element at 0 in values. End procedure.
```

Typed call arguments are checked against the registered parameter types using the same supported assignment conversions as native object initialization. Typed calls may return arithmetic scalars or object pointers. C functions cannot return arrays directly, so an array return type is rejected; return a pointer instead.

A `void` typed procedure may fall through or use bare `Return.`. A non-void typed procedure must currently end with a `Return` statement, and every returned value is checked against its declared return type. This final-statement rule is intentionally conservative until full control-flow definite-return analysis lands.

Procedure signatures are registered before bodies are checked and generated C prototypes are emitted before definitions. Forward calls and mutual recursion therefore do not depend on source order.

Legacy procedures continue using the boxed `PsValue` calling convention and retain their previous permissive argument-value behaviour. Native pointers/arrays are transported through the typed procedure surface instead of being boxed into legacy procedure arguments.

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
          | ElementExpr
          | MemberExpr
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
ElementExpr ::= "Element" "at" Expr "in" Primary
MemberExpr ::= "Member" IDENT "of" Primary
```

Precedence, low to high: `or` → `and` → `not` → comparison → additive → multiplicative → power → primary. Addressing, indirection, native subscripting, list access, length, size/alignment queries, and calls bind as primaries. Parentheses override precedence.

## Types and representation boundary

Legacy PlainSpeak values remain:

- `number` — boxed signed C `long`
- `decimal` — boxed C `double`
- `string` — runtime text
- homogeneous mutable lists of those values

Explicit `Declare` objects instead use native C storage for the `CType` written in source. These two representations are intentionally distinct. Reading a native arithmetic object in a legacy expression boxes its current value; assigning a legacy numeric expression into a native arithmetic object converts it back to that object's C type.

The compiler's structural semantic type system also represents functions, qualifiers, aggregates, enums, C23 bit-precise integers and null pointers as foundations for later syntax. Fixed arrays are now source-spellable native objects; variable-length and incomplete-array source forms remain pending. Representation in the compiler is not itself a claim of supported source capability; `docs/c-compatibility.md` is authoritative about status.

## Known gaps

- `sizeof`/alignment results are boxed into legacy `number`; a first-class unsigned `size_t`-equivalent remains pending.
- Pointer conditions, null pointers, function pointers and the complete C pointer-conversion model remain pending.
- Variable-length arrays, whole-array initializers, unions/bit-fields/flexible structure members, enums, qualifiers, atomics, storage/linkage specifiers, allocation and C23 pointer additions remain pending.
- Legacy Procedure parameters/returns remain boxed and permissive; use typed Procedures for native C signatures. Variadics, function pointers and full prototype-compatibility rules remain pending.
- Heap storage used by string concatenation, list snapshots, and lists is released only at process exit.
- Lists cannot contain lists, native pointers, native arrays, or native aggregates.

Extend the grammar by following the checklist in `AGENTS.md`; grammar, AST, parser, semantic analysis, code generation, runtime behaviour, documentation, and tests must land together.
