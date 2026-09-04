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
       | CallStmt | ProcedureStmt | ReturnStmt | StructureStmt | UnionStmt | EnumerationStmt

SayStmt ::= ("Say" | "Print") Expr "."
SetStmt ::= ("Set" | "Let" | "Make") IDENT "to" Expr "."
DeclareStmt ::= "Declare" IDENT "as" CType NativeInitializer? "."
NativeInitializer ::= "with" "value" Expr
                    | "with" "values" Expr ("followed" "by" Expr)* "done"
                    | "with" "members" IDENT "as" Expr ("followed" "by" IDENT "as" Expr)* "done"
                    | "with" "elements" "at" NUMBER "as" Expr ("followed" "by" "at" NUMBER "as" Expr)* "done"
StoreThroughStmt ::= ("Set" | "Let" | "Make") "value" "at" Expr "to" Expr "."
StoreElementStmt ::= ("Set" | "Let" | "Make") "element" "at" Expr "in" Expr "to" Expr "."
StoreMemberStmt ::= ("Set" | "Let" | "Make") "member" IDENT "of" Expr "to" Expr "."
OrdinaryField ::= "Field" IDENT "as" CType "."
BitField ::= "Bit" "field" IDENT? "as" CType "with" "width" NUMBER "."
FlexibleField ::= "Flexible" "field" IDENT "as" CType "."
StructureMember ::= OrdinaryField | BitField | FlexibleField
UnionMember ::= OrdinaryField | BitField
StructureStmt ::= "Structure" IDENT ":" StructureMember+ "End" "structure" "."
UnionStmt ::= "Union" IDENT ":" UnionMember+ "End" "union" "."
EnumeratorDef ::= "Enumerator" IDENT ("as" ("minus")? NUMBER)? "."
EnumerationStmt ::= "Enumeration" IDENT ":" EnumeratorDef+ "End" "enumeration" "."
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

PlainSpeak exposes the ordinary C scalar family through deterministic prose spellings, plus recursive object-pointer, fixed-array, tagged structure, tagged union, and tagged enumeration types:

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

TypeQualifier ::= "constant" | "volatile" | "restricted" | "atomic"
CType ::= TypeQualifier* CCoreType
CCoreType ::= CScalarType
            | "pointer" "to" CType
            | "array" "of" CType "with" "length" NUMBER
            | "structure" IDENT
            | "union" IDENT
            | "enumeration" IDENT
            | "null" "pointer" "type"
```

The scalar spellings map to C `_Bool`, `char`, `signed char`, `unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long`, `unsigned long`, `long long`, `unsigned long long`, `float`, `double`, and `long double`. Plain `character` remains distinct from both signed and unsigned character types, matching C.

`pointer to` is recursive:

```text
Declare p as pointer to integer. Declare pp as pointer to pointer to integer.
```

A `void` type may be the pointee of a pointer, but a standalone object or array element cannot be `void`. Fixed array bounds are positive whole-number literals. Arrays and pointers may nest recursively, so `pointer to array of integer with length 4` and `array of pointer to integer with length 4` are distinct types and lower to distinct C declarators.

## Native type qualifiers

PlainSpeak exposes C's native type qualifiers with recursive prose spellings:

```text
Declare limit as constant integer.
Declare observed as volatile integer.
Declare cursor as restricted pointer to integer.
Declare shared as atomic integer.
Declare p as pointer to constant integer.
Declare q as constant pointer to integer.
```

The words map directly to C `const`, `volatile`, `restrict`, and `_Atomic`. Qualifiers apply to the **type immediately following them**, so `constant pointer to integer` is a const-qualified pointer while `pointer to constant integer` is a mutable pointer to const-qualified integer. Multiple qualifiers may be combined; repeated qualifiers are idempotent and the AST printer uses the canonical order `constant volatile restricted atomic`.

Reading a qualified scalar follows C value conversion: top-level qualifiers do not become qualifiers on arithmetic results. Pointer conversions may add pointee qualification, such as `pointer to integer` to `pointer to constant integer`, but may not discard it. Nested pointer qualification remains strict, so the unsafe C-style `T **` to `const T **` conversion is not accepted merely because the innermost object can be qualified.

`constant` is enforced through every current native mutation surface: direct `Set`, `Add`/`Subtract`, pointer stores, array element stores and structure/union member stores. Qualifiers on an aggregate propagate to member access as C requires. A const pointer may still modify a mutable pointee; a pointer to const may not.

`restricted` is accepted only on pointers to object types. `atomic` lowers to C11 `_Atomic` and is currently available for complete non-array object types and pointers. Ordinary scalar reads/writes therefore use the backend compiler's real C atomic semantics rather than a PlainSpeak lock or wrapper. Direct member access on an atomic structure/union is rejected because C defines that access as undefined; use whole-object atomic operations when that surface lands. Atomic bit-fields are also rejected by the portable backend.

For array spellings, `constant array of T ...` and `volatile array of T ...` qualify the element type, matching C's array qualification rules.

One lowering boundary remains: direct top-level native objects are declared at C file scope while most PlainSpeak initializers execute later in `main`. A top-level const object with a runtime `with value` initializer is therefore rejected until the constant-initializer tranche can emit that initializer at file scope. Likewise, aggregate initializer forms that currently lower as zero-initialize-then-member-store are rejected when const subobjects or atomic aggregate objects would make those post-declaration stores invalid. Local scalar/pointer const initialization is already emitted directly in the C declaration.

## Null pointer constants and C23 `nullptr_t`

PlainSpeak exposes the C23 predefined null pointer constant as `null pointer` and its distinct scalar type as `null pointer type`:

```text
Declare n as null pointer type with value null pointer.
Declare p as pointer to integer with value null pointer.
Say Convert n to type boolean.
```

`null pointer type` is semantically distinct from every pointer type. Its only valid value is `null pointer`; values of that type convert implicitly to any supported object-pointer type and to `boolean` (as false), but no other type converts to `null pointer type`. Its size and alignment follow `void *`, matching C23's `nullptr_t` requirements.

For the older C99-C17 null-pointer-constant rule, the literal integer `0` is accepted implicitly in pointer initialization, assignment, typed Procedure arguments/returns, equality comparison and conditional-expression branches. Nonzero integers are not implicit pointer conversions; use an explicit `Convert ... to type pointer to ...` where C permits an implementation-defined integer/pointer cast.

Pointer and null-pointer values are valid scalar conditions in `If`, `While`, logical operations and `Choose`. Equality against `0`, `null pointer`, or a `null pointer type` object lowers to native C comparison.

This is still a foundation for the full null-pointer-constant model: arbitrary **integer constant expressions** that evaluate to zero are not yet folded/classified as null pointer constants. Function pointers are also pending, so null conversion is currently exercised against object pointers only.

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

Native arithmetic scalar declarations accept arithmetic initializers and assignments using C assignment conversion at the generated-C boundary. Compatible object pointers may be assigned directly; object-pointer/`void *` compatibility is recognised. Ordinary C integer promotions and usual arithmetic conversions are implemented for the current boolean, enumeration, integer-rank and real-floating families; C23 `_BitInt`, complex arithmetic and remaining edge semantics are separate conformance work.

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

Whole-array assignment remains intentionally unavailable. Fixed arrays can now be initialized positionally or with zero-based element designators at declaration time. Variable-length arrays, pointer truthiness, null pointers, function pointers and allocated storage remain later work.

## Native structures and members

A tagged structure definition creates real C aggregate layout:

```text
Structure point: Field x as integer. Field y as integer. End structure. Declare p as structure point. Set member x of p to 10. Say Member x of p.
```

Structure, union, and enumeration tags use C's shared tag namespace and are pre-registered as incomplete before definition checking. This permits self-referential and forward pointers such as `pointer to structure node`, while a recursive or forward structure used **by value** must already be complete at that definition point.

`Member name of base` reads a field from either a structure object or a pointer to one. `Set member name of base to value.` writes a non-array field after normal native assignment checking. The compiler chooses C `.` or `->` from the semantic base type; the prose syntax does not expose that punctuation distinction.

Fixed arrays and already-complete structures may be fields. Array fields can be reached with native `Element at`, including nested expressions. Whole-array member assignment is still intentionally rejected.

Native structures can be copied by assignment, initialized positionally or with named member designators, and transported by value through typed Procedures when their tags match. Compound literals and anonymous aggregate members remain separate work.

## Native unions

A tagged union definition uses the same field spelling but lowers to real C `union` layout:

```text
Union value: Field whole as integer. Field fraction as decimal. End union. Declare v as union value. Set member whole of v to 42. Say Member whole of v.
```

Unions share C's tag namespace with structures, so `Structure value` and `Union value` cannot both be defined. Like structures, union tags are known as incomplete before their fields are checked, allowing recursive and forward **pointers** while rejecting recursive by-value members.

`Member name of base` and `Set member name of base to value.` work for union objects and pointers as well as structures. Union values may be copied and passed/returned by value through typed Procedures when their union types match.

PlainSpeak does **not** add a runtime active-member discriminator. Writing one member and then reading another follows whatever semantics the generated C program has on the target/compiler; the language does not reinterpret C unions as tagged variants. The conformance tests therefore validate same-member reads/writes and layout, not type-punning assumptions.

Unions can be initialized through their first member positionally or one named member designator. Compound literals and anonymous aggregate members remain separate work.

## Bit-fields and flexible array members

Bit-fields lower to native C bit-field declarations rather than masks maintained by the PlainSpeak runtime:

```text
Structure flags: Bit field low as unsigned integer with width 3. Bit field as unsigned integer with width 0. Bit field high as unsigned integer with width 2. End structure.
```

A named bit-field must have a positive width. An unnamed width-0 bit-field is the prose form of C's allocation-unit/alignment separator. Existing integer ranks, `boolean`, and completed enumerations are accepted where the target C compiler supports them. PlainSpeak validates widths against its current backend scalar-width model; exhaustive detection of implementation-defined bit-field bases and enum representation remains a conformance boundary. Named bit-fields participate in `Member` reads/stores and aggregate initialization like scalar members. Unnamed bit-fields are skipped by positional initialization and cannot be named by a designator.

C does not permit `sizeof` on a bit-field expression, so `Size of Member flags of value` is rejected even though the member has an integer semantic type. The current `Address of` syntax only accepts declared object names, so it cannot incorrectly take a bit-field address.

A flexible array member is structure-only and uses an incomplete trailing C array:

```text
Structure packet: Field length as unsigned integer. Flexible field data as unsigned character. End structure.
```

The flexible member must be last and the structure must contain at least one other named member. A union cannot declare a flexible array member directly, but it may contain a structure that has one. Such a union inherits C's restriction: it may not then be a structure member or an array element, and that restriction propagates recursively through containing unions. A structure containing a flexible member may exist as a native object and has normal C `sizeof` semantics (the flexible tail contributes no elements), but it cannot itself be a structure member or array element. Use a pointer when one of those placements is required.

`Member data of packet` retains incomplete-array semantics and can decay for element access once suitably extended storage exists. The current language does not yet provide allocation sized beyond the base structure, so direct flexible-tail element access is not presented as a safe allocation facility. Flexible members are never aggregate initializer targets; their storage is supplied separately, matching the C object model.

## Native enumerations

A tagged enumeration defines named integral constants and a real C `enum` type:

```text
Enumeration color: Enumerator red. Enumerator green as 5. Enumerator blue. End enumeration. Declare current as enumeration color with value Enumerator blue of color. Say current.
```

An implicit first Enumerator has value 0. Each later implicit Enumerator is one greater than the previous value; an explicit signed whole-number literal resets that sequence. Repeated **values** are permitted, but Enumerator names within one Enumeration must be unique.

Enumerator expressions are qualified in PlainSpeak as `Enumerator name of enumeration`. This deliberately avoids copying C's global ordinary-identifier namespace for source enumerator constants. Generated C still uses a real `enum`; the compiler qualifies/mangles each generated enumerator name so separate PlainSpeak enumerations can safely reuse names.

Enumeration objects are native integral scalar objects. They can be addressed, dereferenced, assigned from compatible arithmetic values, used in arithmetic/comparison, passed/returned through typed Procedures, and queried with `Size of` / `Alignment of type`. Enumerator expressions model the C99-C17 integer-constant behaviour and currently use the backend's C `int` range.

This tranche intentionally limits explicit enumerator values to signed whole-number literals representable by C `int`. General integer constant expressions and C23 fixed underlying enumeration types / wider enumerator rules remain separate work.

## Aggregate initialization

Explicit native declarations support positional and designated aggregate initialization without exposing C brace syntax.

Positional initialization uses declaration order:

```text
Structure point: Field x as integer. Field y as integer. End structure. Declare p as structure point with values 3 followed by 4 done. Declare a as array of integer with length 4 with values 1 followed by 2 done.
```

For structures, positional values map to fields in definition order. For arrays, they map from zero-based element 0 upward. A positional union initializer may supply at most one value and therefore initializes its first member.

Named member designators are available for structures and unions:

```text
Declare p as structure point with members y as 8 done. Union value: Field whole as integer. Field fraction as decimal. End union. Declare u as union value with members fraction as 2.5 done.
```

Array index designators use the same zero-based native indexing convention:

```text
Declare a as array of integer with length 4 with elements at 3 as 9 followed by at 1 as 7 done.
```

Omitted structure members and array elements are zero-initialized. Automatic aggregate objects use an explicit generated C zero-initializer before the selected member/element stores; file-scope native objects already have C static zero initialization. This keeps PlainSpeak initializer expressions free to use runtime expressions instead of pretending they are all C translation-time constants.

Designators must be unique in this tranche, array indexes must be in range, and a union initializer selects exactly one member. Nested array aggregate initialization is not yet recursive: an array-valued field/element needs a later nested-initializer facility rather than assignment syntax. Ordinary `with value` remains the scalar/native assignment-style initializer and can still copy compatible complete structures/unions or consume typed Procedure results; arrays use the aggregate forms instead.

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
AndExpr ::= BitwiseOrExpr ("and" BitwiseOrExpr)*
BitwiseOrExpr ::= BitwiseXorExpr ("bitwise" "or" BitwiseXorExpr)*
BitwiseXorExpr ::= BitwiseAndExpr ("bitwise" "xor" BitwiseAndExpr)*
BitwiseAndExpr ::= NotExpr ("bitwise" "and" NotExpr)*
NotExpr ::= "not" NotExpr | "bitwise" "not" NotExpr | Comparison
Comparison ::= ShiftExpr ("is" Comparator ShiftExpr)?

Comparator ::= "greater" "than"
             | "less" "than"
             | "equal" "to"
             | "not" "equal" "to"
             | "greater" "than" "or" "equal" "to"
             | "less" "than" "or" "equal" "to"

ShiftExpr ::= Additive (("shifted" ("left" | "right") "by") Additive)*
Additive ::= Multiplicative (("plus" | "minus") Multiplicative)*
Multiplicative ::= Power (("times" | "divided by" | "mod" | "modulo") Power)*
Power ::= Primary ("to" "the" "power" "of" Primary)*

Primary ::= NUMBER | FLOAT | STRING | IDENT | "true" | "false" | "null" "pointer"
          | "minus" Primary
          | "(" Expr ")"
          | "Choose" Expr "when" Expr "otherwise" Expr
          | ("Increment" | "Decrement") ("before" | "after") Primary
          | "Convert" Primary "to" "type" CType
          | "Address" "of" IDENT
          | "Value" "at" Primary
          | ElementExpr
          | MemberExpr
          | EnumeratorExpr
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
EnumeratorExpr ::= "Enumerator" IDENT "of" IDENT
```

Precedence, low to high: `or` → `and` → `bitwise or` → `bitwise xor` → `bitwise and` → `not` / `bitwise not` → comparison → shifts → additive → multiplicative → power → primary. `Choose`, `Increment/Decrement before/after`, and `Convert` are explicit expression forms; convert a parenthesized expression when the operand should include lower-precedence operators. Addressing, indirection, native subscripting, list access, length, size/alignment queries, and calls bind as primaries. Parentheses override precedence.

## C arithmetic conversions and bitwise operators

For native arithmetic expressions, PlainSpeak now follows C's integer promotions and usual arithmetic conversions instead of collapsing every operation through the legacy boxed `long`/`double` runtime. `boolean`, character, short integer and current enumeration values promote as C requires; mixed signed/unsigned integer operands use rank and representable-range rules, and floating operands preserve `float` / `double` / `long double` rank.

The bitwise forms are `bitwise and`, `bitwise xor`, `bitwise or`, and unary `bitwise not`. Shift expressions use `shifted left by` and `shifted right by`. Bitwise operands must be integer types, and each shift operand undergoes integer promotion; the result type of a shift is the promoted left operand. Modulo is likewise restricted to integer operands.

These expressions lower directly to C `&`, `^`, `|`, `~`, `<<`, `>>`, and the ordinary arithmetic operators, so the backend compiler performs the same target-specific conversion and execution rules represented by sema. PlainSpeak does not currently promise to diagnose every run-time undefined or implementation-defined shift case (for example, a negative or excessive shift count); those remain part of the broader C execution-semantics conformance work.

## Conditional expressions

`Choose A when C otherwise B` provides C's conditional-expression capability and lowers directly to `C ? A : B`:

```text
Say Choose 10 when ready otherwise 20.
Declare selected as pointer to constant integer with value Choose p when flag otherwise cp.
```

The condition must currently have a C scalar type: an arithmetic value or object pointer. As with C's `?:`, only the selected branch is evaluated by the generated program.

When both branches are arithmetic, the usual arithmetic conversions determine the result type. Compatible pointer branches produce a composite pointer type: pointed-to qualifiers are combined, and an object pointer paired with `void *` produces the appropriately qualified `void *` result. Two identical structure or union branch types produce that aggregate type by value.

This is still a foundation rather than complete C conditional-expression parity. Integer null-pointer constants are not yet tracked as constant-expression metadata, function pointers are not source-spellable, and void-valued branch expressions need the future discard/void-expression surface.

## Prefix and postfix increment/decrement

PlainSpeak exposes C's four increment/decrement expression forms with explicit value-timing words:

```text
Say Increment before x.
Say Increment after x.
Say Decrement before x.
Say Decrement after x.
```

`before` is the prefix form: the object is changed first and the expression yields the updated value. `after` is the postfix form: the expression yields the previous value and the side effect updates the object afterwards, following C's sequencing rule for postfix increment/decrement.

The operand must be a native modifiable lvalue of real arithmetic type or a pointer to a complete object type. Native variables, dereferences, array elements and structure/union members (including named bit-fields) are supported. Boxed PlainSpeak variables, temporaries, whole arrays, aggregates, const-qualified objects, `void *` and pointers to incomplete objects are rejected before C emission.

The compiler lowers these forms directly to C `++` and `--`. Pointer increments therefore use the pointed-to type's element size, and C11 atomic operands use the implementation's native read-modify-write semantics. The expression result is a non-lvalue value with top-level qualifiers removed.

## Explicit C conversions

`Convert value to type CType` is PlainSpeak's explicit C cast capability:

```text
Say Convert 3.75 to type integer.
Declare vp as pointer to void with value Convert Address of x to type pointer to void.
Declare p as pointer to integer with value Convert vp to type pointer to integer.
```

The target must currently be a scalar C type. Arithmetic-to-arithmetic conversions lower directly to C casts. Object pointers may be explicitly converted to other object-pointer types, integers may be explicitly converted to pointers, and pointers may be explicitly converted to integer types; the implementation-defined details of pointer/integer representation remain properties of the target C implementation. Any scalar value may be converted to `boolean`, matching C's zero/nonzero scalar conversion.

Floating-point values cannot be converted directly to pointers, and arrays, functions, structures, unions, strings and lists are not scalar cast targets/operands. Casts to a completed enumeration type are accepted; incomplete enum targets are rejected before code generation.

`Convert ... to type void` is not exposed as a value expression yet because PlainSpeak currently has no standalone discard-expression statement. That specific C use remains pending rather than pretending a void value can flow through `Say` or assignment.

## Types and representation boundary

Legacy PlainSpeak values remain:

- `number` — boxed signed C `long`
- `decimal` — boxed C `double`
- `string` — runtime text
- homogeneous mutable lists of those values

Explicit `Declare` objects instead use native C storage for the `CType` written in source. These two representations are intentionally distinct. Reading a native arithmetic object in a legacy expression boxes its current value; assigning a legacy numeric expression into a native arithmetic object converts it back to that object's C type.

The compiler's structural semantic type system also represents functions, qualified recursive native types, aggregates, enums, C23 bit-precise integers and null pointers. Qualifiers are now source-spellable; representation of the remaining shapes is still only a foundation until their syntax/lowering lands. Fixed arrays are now source-spellable native objects; variable-length and incomplete-array source forms remain pending. Representation in the compiler is not itself a claim of supported source capability; `docs/c-compatibility.md` is authoritative about status.

## Known gaps

- `sizeof`/alignment results are boxed into legacy `number`; a first-class unsigned `size_t`-equivalent remains pending.
- Function pointers and the complete C pointer-conversion model remain pending. Null pointer values/literal-zero constants are implemented, but general integer constant-expression recognition for zero-valued null pointer constants is not yet complete.
- Variable-length arrays, nested aggregate initializers, compound literals, anonymous aggregate members, enum constant-expression/underlying-type extensions, storage/linkage specifiers, allocation and C23 pointer additions remain pending. Atomic memory-order APIs, fences and lock-free queries are also still pending beyond the native `atomic` object foundation.
- Legacy Procedure parameters/returns remain boxed and permissive; use typed Procedures for native C signatures. Variadics, function pointers and full prototype-compatibility rules remain pending.
- Heap storage used by string concatenation, list snapshots, and lists is released only at process exit.
- Lists cannot contain lists, native pointers, native arrays, or native aggregates.

Extend the grammar by following the checklist in `AGENTS.md`; grammar, AST, parser, semantic analysis, code generation, runtime behaviour, documentation, and tests must land together.
