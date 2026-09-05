# C99-C23 capability parity

PlainSpeak's systems-language target is the union of program capabilities provided by C99, C11, C17 and C23. This is a **capability-parity target**, not a claim that PlainSpeak accepts C source syntax. PlainSpeak keeps its prose grammar while gaining equivalent types, object/memory semantics, control flow, translation/linkage facilities, concurrency and hosted/freestanding library access.

The normative standards are ISO/IEC 9899 revisions. For implementation work we use WG14's public working material, especially N1256 for the consolidated C99 text, N1570 for C11, the C17 defect-resolution lineage, and the C23 project/draft lineage. C23 was published as ISO/IEC 9899:2024.

The machine-readable source of truth is [`tests/conformance/c99-c23.json`](../tests/conformance/c99-c23.json). CI validates it with `scripts/check_c_conformance.py`. Status values mean:

- **implemented** — first-class usable capability with a regression test.
- **foundation** — compiler/runtime representation or a partial surface exists, but the C capability is not complete.
- **planned** — explicitly missing and therefore cannot be accidentally implied by documentation.
- **non_applicable** — allowed only with a written rationale showing no program capability is lost.

An arbitrary-C escape hatch does **not** count as parity.

## Type system and object model

| ID | Status | Scope |
|---|---|---|
| `types.integer-model` | foundation | Structural integer ranks/signedness and the ordinary C integer promotions/usual signed-unsigned conversions are executable; C23 `_BitInt` rank interactions and remaining conversion edge cases are incomplete. |
| `types.bitint` | foundation | C23 bit-precise integer type is structurally representable. |
| `types.floating-model` | foundation | Float/double/long-double ranks are structurally represented and native objects can use all three; the full C floating environment/model is still incomplete. |
| `types.complex` | planned | Complex objects and arithmetic. |
| `types.boolean` | foundation | Native `_Bool` objects are spellable; legacy true/false literals still preserve numeric compatibility. |
| `types.nullptr` | foundation | C23 `null pointer` and `null pointer type` are source-spellable; the distinct scalar type has void-pointer/character-pointer-compatible layout, C23 default initialization, native object storage, and supported null-constant/pointer/bool conversions enforced semantically. |
| `types.object-representation` | foundation | Explicit scalar, pointer, fixed-array, tagged-structure, tagged-union and tagged-enumeration objects use real C storage, address, size and target layout; padding/effective-type rules and complete lifetime semantics remain pending. |
| `types.sizeof-alignof` | foundation | Type queries plus object-expression `Size of` preserve fixed-array extent and complete structure/union/enum layout as well as scalar/pointer layout; requested alignment and native `size_t` remain pending. |
| `types.qualifiers` | foundation | Recursive `constant`/`volatile`/`restricted`/`atomic` source qualifiers lower to native C const/volatile/restrict/_Atomic, preserve pointer placement, enforce const modifiability and directional pointee qualification; full typedef/array compatibility details and constant-initializer lowering remain pending. |
| `types.pointers` | foundation | Recursive object pointers support address/dereference, array decay, element-scaled +/- arithmetic, pointer difference/comparison, +=/-= offsets, pointer-level qualifiers and qualifier-adding pointee conversions; null/function pointers and complete compatibility rules remain pending. |
| `types.function-types` | implemented | Explicit typed Procedure parameters/returns lower to native C function types with recursive native qualifiers, C array-parameter adjustment, checked calls, generated prototypes, forward calls and mutual recursion. Variadics, function pointers and complete compatible-type rules remain pending in their own rows. |
| `types.arrays` | foundation | Positive fixed-bound native arrays are source-spellable with C storage, sizeof, subscript/store and ordinary array-to-pointer decay; VLAs/incomplete source declarations and whole-array initialization remain pending. |
| `types.vla` | planned | C99 variable-length and variably modified types. |
| `types.structures` | foundation | Tagged structures have source definitions, completeness checking, native layout, self/forward pointers, by-value transport/member access, bit-fields and flexible-array tails; anonymous members remain pending. |
| `types.bit-fields` | foundation | Named/unnamed native C bit-fields support width checks, width-0 unnamed separators, member access/store and initialization, with completed enums required before layout; C23 _BitInt source types and exhaustive implementation-defined base/enum-representation detection remain pending. |
| `types.flexible-array-members` | foundation | C99 trailing flexible structure members have native incomplete-array layout, sizeof/completeness and initializer constraints, including recursive propagation through unions for structure-member/array-element restrictions; allocation of extended objects remains pending. |
| `types.unions` | foundation | Tagged unions have source definitions, completeness checking, native layout, self/forward pointers, by-value transport/member access and native bit-fields; unions may contain flexible-array structures while inheriting C's placement restrictions; anonymous members remain pending. |
| `types.enumerations` | foundation | Tagged enumerations have source definitions, implicit/explicit int-range enumerators, native enum storage, qualified enumerator expressions and typed transport; general integer constant expressions and C23 fixed underlying/wider rules remain pending. |
| `types.aliases` | planned | typedef-equivalent aliases. |
| `types.typeof` | planned | C23 `typeof` / `typeof_unqual` capability. |
| `types.auto-inference` | planned | C23 inferred `auto` capability. |
| `types.constexpr` | planned | C23 constexpr object capability. |

## Expressions and conversions

| ID | Status |
|---|---|
| `expr.integer-constant-expressions` | foundation |
| `expr.integer-promotions` | foundation |
| `expr.value-categories` | foundation |
| `expr.arithmetic` | foundation |
| `expr.bitwise` | foundation |
| `expr.shifts` | foundation |
| `expr.assignment` | foundation |
| `expr.increment-decrement` | foundation |
| `expr.address-indirection` | foundation |
| `expr.subscript-member` | foundation |
| `expr.casts` | foundation |
| `expr.conditional` | foundation |
| `expr.sequencing` | planned |
| `expr.function-calls` | foundation |
| `expr.compound-literals` | planned |
| `expr.generic-selection` | planned |
| `expr.nullptr-conversions` | foundation |

Explicit native objects model C modifiable-lvalue constraints: const-qualified objects and aggregates containing const subobjects cannot be mutated; pointer dereference, fixed-array elements and structure/union members (including named bit-fields) preserve effective const/volatile qualification. Arrays decay to element pointers in ordinary value contexts but retain extent for `Size of` and `Address of`. Pointer +/- integer, same-element-type pointer difference/comparison and pointer +=/-= offsets are supported. This remains **foundation** because function decay, null pointers, complete conversions, anonymous members, sequencing and the full usual arithmetic conversions are not complete.

PlainSpeak now classifies and evaluates a source-spellable subset of C integer constant expressions at translation time: integer/boolean constants, unary integer operators, integer binary arithmetic/bitwise/shift/comparison/logical operators and conditional expressions, including short-circuit unevaluated branches. This powers zero-valued null pointer constants without treating runtime zero values as constants. The row remains **foundation** until enumerator references, constant `sizeof`/`alignof`, integer casts, C23 `constexpr` names and the remaining extended rules are represented.

Native arithmetic expressions now apply C integer promotions and usual arithmetic conversions across the ordinary integer and real-floating families, and lower directly to C operators. Bitwise AND/XOR/OR/complement and shifts are source-spellable with promoted result types. This remains **foundation** because C23 `_BitInt` conversion rank interactions, complex arithmetic, full constant-expression overflow analysis, and exhaustive undefined/implementation-defined shift behavior are not yet covered.

Conditional expressions are now source-spellable and lower directly to C `?:`. Arithmetic branches use usual arithmetic conversions; compatible object-pointer branches compose pointed-to qualifiers and `void *`; identical structure/union branches are transported by value. This remains **foundation** because integer null-pointer constants, function pointers, void-valued branches and the remaining exhaustive composite-type rules are pending.

Prefix/postfix increment and decrement are now source-spellable over native modifiable arithmetic/pointer lvalues, including array elements, dereferences, named bit-fields and C11 atomic scalar objects. The operators lower directly to C, preserving prefix/postfix value timing and pointer scaling. This remains **foundation** because function-pointer operands are not yet source-spellable and the repository has not yet completed the broader C sequencing model.

Zero-valued integer constant expressions from the current compile-time evaluator and C23 `nullptr_t` values now participate in object-pointer initialization/assignment, typed calls/returns, equality, scalar conditions and conditional expressions. C23 `nullptr_t` remains a distinct semantic scalar type and lowers through a C11-compatible pointer-sized backend representation. This remains **foundation** because the remaining integer constant-expression forms and function-pointer null conversions await their respective tranches.

Explicit scalar conversions are now source-spellable and lower to native C casts across arithmetic↔arithmetic, object-pointer↔object-pointer, integer↔pointer and scalar→boolean categories. C23 null-pointer constants may also explicitly convert to `null pointer type`, while `nullptr_t` converts to supported object pointers and boolean. This remains **foundation** because cast-to-void discard expressions, function pointers, general zero-valued integer constant-expression recognition, and exhaustive implementation-defined pointer/integer guarantees are still pending.

## Declarations, storage and linkage

| ID | Status |
|---|---|
| `decl.explicit-declarations` | foundation |
| `decl.storage-duration` | foundation |
| `decl.linkage` | planned |
| `decl.storage-specifiers` | planned |
| `decl.initializers` | foundation |
| `decl.designated-initializers` | foundation |
| `decl.empty-initialization` | foundation | Native declarations of scalar, pointer, fixed-array, structure, union, enumeration and decimal objects with no initializer clause are zero-initialized in the generated C: integer/boolean/enumeration objects get 0, pointers get the platform null, decimal objects get 0.0, fixed-array elements and aggregate members are recursively zero-initialized. Legacy boxed declarations and the C23 `{}` empty-brace spelling remain pending. |
| `decl.static-assert` | planned |
| `decl.attributes` | planned |

`Declare` now introduces native scalar (including complete enumerations), pointer, fixed-array and complete tagged-aggregate objects independently of assignment. Direct top-level declarations use static storage duration in the generated translation unit; block/procedure declarations use automatic storage duration. Scalar/pointer assignment-style initializers plus positional aggregate, named member-designated, and array index-designated initialization are type-checked. Omitted aggregate slots are zeroed. Native declarations with no initializer at all are also zero-initialized (C23-style omitted-initializer behaviour for the native subset); user-controlled linkage, `static`/`extern`/thread storage, allocated storage, the C23 `{}` empty-brace spelling and full C constant-initializer rules remain missing.

## Statements and control flow

| ID | Status |
|---|---|
| `control.if` | implemented |
| `control.while` | implemented |
| `control.do-while` | implemented |
| `control.for` | implemented |
| `control.switch` | implemented |
| `control.break` | implemented |
| `control.continue` | implemented |
| `control.goto-labels` | implemented |
| `control.return` | implemented |

`Do: ... End do while condition.` now supplies C's post-test loop semantics with native scalar conditions, one guaranteed first iteration, and direct C `do { ... } while (...);` lowering. `Break.` and `Continue.` lower directly to C in every current PlainSpeak loop form and, for `Break.`, inside `Switch` bodies, including when nested inside conditional blocks. Semantic analysis rejects either statement outside an allowed context and maintains the breakable vs. loop context model through nested scopes. `control.break` is **implemented**: it exits loops and switch statements, is rejected outside breakable contexts, and lowers directly to C `break;`. `control.continue` is **implemented**: it is loop-only and, inside a switch contained in a loop, continues the enclosing loop exactly as C does. `control.return` is **implemented**: a typed non-void Procedure is rejected (E0018) when any control path can reach the function end without returning, via a per-path CFG analysis covering If/Else, Switch, While/DoWhile/Repeat/For/ForEach loops, Break/Continue/Goto/Label, and known-non-empty ranges. Loop bodies that return inside have their generated C guarded with a dead trailing return so the lowered function is well-formed.

PlainSpeak's `For each` is a language extension and is not counted as a replacement for C's `for`. `For i from bound to bound:` / `For i from bound down to bound:` supplies the common bounded counting form of C's `for` with a native `long` loop variable, once-evaluated bounds, and `Break.`/`Continue.` lowering to C `break;`/`continue;`. `control.for` is **implemented**: the bounded C99 for-loop surface that PlainSpeak exposes is complete and end-to-end tested; the C three-clause `for(init; cond; post)` form is intentionally not a PlainSpeak surface (the language has no comma operator to spell the three clauses).

`Switch value: When constant: ... Otherwise: ... End switch.` lowers directly to a C `switch` statement with integer `case` labels and an optional `default`. The condition must be integral; each `When` clause requires an integer constant expression; duplicate `When` values and multiple `Otherwise` clauses are rejected; clause bodies share one block scope and fall through unless ended with `Break.`, matching C's semantics. `control.switch` is **implemented**: the C99 switch surface used by PlainSpeak is complete and end-to-end tested; C23 range case labels and exhaustive implementation-defined label-type edge cases are not part of the grammar.

`Label name.` and `Go to name.` lower directly to C `label:` and `goto`. Labels are function-scoped exactly as in C: forward and backward jumps work, labels resolve only within the enclosing procedure or top-level body, duplicates are rejected, and jumping over an automatic object's declaration leaves that object indeterminate per C. `control.goto-labels` is **implemented**: the C99 goto surface used by PlainSpeak is complete and end-to-end tested. The VLA-in-scope prohibition is vacuously satisfied because PlainSpeak has no variable-length types, and C23 label attributes are not part of the grammar.

## Functions

| ID | Status |
|---|---|
| `func.typed-signatures` | foundation |
| `func.prototypes` | foundation |
| `func.variadic` | foundation |
| `func.recursion` | foundation |
| `func.inline` | planned |
| `func.noreturn` | planned |

Native pointers deliberately do not pass through legacy untyped `Procedure` parameters or returns yet; typed signatures/function pointers are the next required function-model layer.

Typed Procedures now have explicit native parameter and return types, recursive native qualifiers, checked calls, C array-parameter adjustment, generated prototypes, forward calls and mutual recursion. Typed `void` and value returns are checked. `func.typed-signatures`, `func.prototypes`, `func.recursion` and `types.function-types` are **implemented** because the C99 function-type surface used by PlainSpeak is complete and end-to-end tested; variadic definitions/calls, function pointers, C's full compatible-type/prototype rules and complete path-sensitive return analysis remain pending in their own rows.

## Translation and preprocessing capability

PlainSpeak does not need to copy C's token-oriented preprocessor syntax, but it must provide equivalent compile-time/program-building capability where C programs depend on it.

| ID | Status |
|---|---|
| `pp.translation-units` | planned |
| `pp.header-interop` | planned |
| `pp.conditional-compilation` | planned |
| `pp.macros` | planned |
| `pp.variadic-vaopt` | planned |
| `pp.elifdef` | planned |
| `pp.warning` | planned |
| `pp.embed` | planned |
| `pp.predefined-environment` | planned |
| `pp.pragma` | planned |

## Concurrency and C memory model

| ID | Status |
|---|---|
| `concurrency.atomics` | foundation |
| `concurrency.fences` | planned |
| `concurrency.lock-free` | planned |
| `concurrency.threads` | planned |
| `concurrency.thread-local` | planned |
| `concurrency.sync` | planned |
| `concurrency.memory-model` | planned |

Native `atomic` objects currently lower to real C11 `_Atomic` objects. Ordinary reads, simple assignments and stores through atomic-qualified pointers therefore use the C compiler's native default atomic semantics. This is only a foundation: explicit memory-order selection, the atomic RMW/API families, fences, lock-free queries, thread-local storage, threads/synchronization, and full happens-before/data-race conformance remain pending.

## Hosted C library

Each header row ultimately expands into per-facility entries as bindings are implemented. A header is not considered complete because one or two functions happen to exist in the current runtime.

| ID | Status | C surface |
|---|---|---|
| `lib.assert` | planned | `<assert.h>` |
| `lib.complex` | planned | `<complex.h>` |
| `lib.ctype` | planned | `<ctype.h>` |
| `lib.errno` | planned | `<errno.h>` |
| `lib.fenv` | planned | `<fenv.h>` |
| `lib.float` | planned | `<float.h>` |
| `lib.inttypes` | planned | `<inttypes.h>` |
| `lib.limits` | planned | `<limits.h>` |
| `lib.locale` | planned | `<locale.h>` |
| `lib.math` | foundation | `<math.h>`; current runtime exposes only a small subset. |
| `lib.setjmp` | planned | `<setjmp.h>` |
| `lib.signal` | planned | `<signal.h>` |
| `lib.stdalign` | planned | `<stdalign.h>` / alignment spellings |
| `lib.stdarg` | planned | `<stdarg.h>` |
| `lib.stdatomic` | planned | `<stdatomic.h>` |
| `lib.stdbool` | foundation | `<stdbool.h>` / C23 boolean spellings |
| `lib.stddef` | planned | `<stddef.h>` |
| `lib.stdint` | planned | `<stdint.h>` |
| `lib.stdio` | foundation | `<stdio.h>`; `Say`/`Read` are only a small high-level subset. |
| `lib.stdlib` | planned | `<stdlib.h>` |
| `lib.stdnoreturn` | planned | `<stdnoreturn.h>` / C23 spelling |
| `lib.string` | planned | `<string.h>` |
| `lib.tgmath` | planned | `<tgmath.h>` |
| `lib.threads` | planned | `<threads.h>` |
| `lib.time` | planned | `<time.h>` |
| `lib.uchar` | planned | `<uchar.h>` |
| `lib.wchar` | planned | `<wchar.h>` |
| `lib.wctype` | planned | `<wctype.h>` |
| `lib.stdbit` | planned | C23 `<stdbit.h>` |
| `lib.stdckdint` | planned | C23 `<stdckdint.h>` |

## Conformance rules

1. The manifest must remain valid JSON with unique feature IDs and recognised C revisions/statuses.
2. Every **foundation** or **implemented** feature must reference at least one real test file. CI verifies the path exists.
3. Every manifest ID must appear in this document, preventing the human and machine views from silently drifting apart.
4. **non_applicable** requires a machine-readable rationale.
5. Moving a row from planned → foundation/implemented requires tests in the same change.
6. A generated-C implementation detail does not become a supported PlainSpeak capability until the frontend semantics and tests expose it intentionally.

## Implementation sequence

The compiler now has structural C-capable semantic types, scalar type/size/alignment queries, and the first real C object layer. Explicit native scalar/pointer declarations carry actual C storage; semantic analysis retains those types into codegen; object addresses, dereference/store-through, pointer-to-pointer composition and object-expression `sizeof` are first-class source capabilities.

The next milestones are fixed arrays plus subscript/decay/pointer arithmetic, then typed function signatures/function pointers, aggregates/initializers, qualifiers/storage/linkage and the remainder of the C99 expression/control-flow model. C11/C17/C23 facilities build on that object model rather than being disconnected runtime tricks.
