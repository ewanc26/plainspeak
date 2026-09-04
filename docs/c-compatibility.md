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
| `types.integer-model` | foundation | Structural integer ranks/signedness exist and explicit native declarations can use the ordinary C integer family; full promotions/conversions remain incomplete. |
| `types.bitint` | foundation | C23 bit-precise integer type is structurally representable. |
| `types.floating-model` | foundation | Float/double/long-double ranks are structurally represented and native objects can use all three; the full C floating environment/model is still incomplete. |
| `types.complex` | planned | Complex objects and arithmetic. |
| `types.boolean` | foundation | Native `_Bool` objects are spellable; legacy true/false literals still preserve numeric compatibility. |
| `types.nullptr` | foundation | C23 null-pointer type can be represented; source literal/conversions remain pending. |
| `types.object-representation` | foundation | Explicit scalar, pointer, fixed-array, tagged-structure, tagged-union and tagged-enumeration objects use real C storage, address, size and target layout; padding/effective-type rules and complete lifetime semantics remain pending. |
| `types.sizeof-alignof` | foundation | Type queries plus object-expression `Size of` preserve fixed-array extent and complete structure/union/enum layout as well as scalar/pointer layout; requested alignment and native `size_t` remain pending. |
| `types.qualifiers` | foundation | Recursive `constant`/`volatile`/`restricted`/`atomic` source qualifiers lower to native C const/volatile/restrict/_Atomic, preserve pointer placement, enforce const modifiability and directional pointee qualification; full typedef/array compatibility details and constant-initializer lowering remain pending. |
| `types.pointers` | foundation | Recursive object pointers support address/dereference, array decay, element-scaled +/- arithmetic, pointer difference/comparison, +=/-= offsets, pointer-level qualifiers and qualifier-adding pointee conversions; null/function pointers and complete compatibility rules remain pending. |
| `types.function-types` | foundation | Explicit typed Procedure parameters/returns now lower to native C function types; variadics, function pointers and the complete compatibility rules remain pending. |
| `types.arrays` | foundation | Positive fixed-bound native arrays are source-spellable with C storage, sizeof, subscript/store and ordinary array-to-pointer decay; VLAs/incomplete source declarations and whole-array initialization remain pending. |
| `types.vla` | planned | C99 variable-length and variably modified types. |
| `types.structures` | foundation | Tagged structures have source definitions, completeness checking, native layout, self/forward pointers, by-value transport/member access, bit-fields and flexible-array tails; anonymous members remain pending. |
| `types.bit-fields` | foundation | Named/unnamed native C bit-fields support target-width checks, width-0 unnamed separators, member access/store and initialization; C23 _BitInt source types and exhaustive implementation-defined base-type feature detection remain pending. |
| `types.flexible-array-members` | foundation | C99 trailing flexible structure members have native incomplete-array layout, sizeof/completeness and embedding/initializer constraints; allocation of extended objects remains pending. |
| `types.unions` | foundation | Tagged unions have source definitions, completeness checking, native layout, self/forward pointers, by-value transport/member access and native bit-fields; anonymous members remain pending. |
| `types.enumerations` | foundation | Tagged enumerations have source definitions, implicit/explicit int-range enumerators, native enum storage, qualified enumerator expressions and typed transport; general integer constant expressions and C23 fixed underlying/wider rules remain pending. |
| `types.aliases` | planned | typedef-equivalent aliases. |
| `types.typeof` | planned | C23 `typeof` / `typeof_unqual` capability. |
| `types.auto-inference` | planned | C23 inferred `auto` capability. |
| `types.constexpr` | planned | C23 constexpr object capability. |

## Expressions and conversions

| ID | Status |
|---|---|
| `expr.integer-promotions` | planned |
| `expr.value-categories` | foundation |
| `expr.arithmetic` | foundation |
| `expr.bitwise` | planned |
| `expr.shifts` | planned |
| `expr.assignment` | foundation |
| `expr.increment-decrement` | planned |
| `expr.address-indirection` | foundation |
| `expr.subscript-member` | foundation |
| `expr.casts` | planned |
| `expr.conditional` | planned |
| `expr.sequencing` | planned |
| `expr.function-calls` | foundation |
| `expr.compound-literals` | planned |
| `expr.generic-selection` | planned |
| `expr.nullptr-conversions` | planned |

Explicit native objects model C modifiable-lvalue constraints: const-qualified objects and aggregates containing const subobjects cannot be mutated; pointer dereference, fixed-array elements and structure/union members (including named bit-fields) preserve effective const/volatile qualification. Arrays decay to element pointers in ordinary value contexts but retain extent for `Size of` and `Address of`. Pointer +/- integer, same-element-type pointer difference/comparison and pointer +=/-= offsets are supported. This remains **foundation** because function decay, null pointers, complete conversions, anonymous members, sequencing and the full usual arithmetic conversions are not complete.

## Declarations, storage and linkage

| ID | Status |
|---|---|
| `decl.explicit-declarations` | foundation |
| `decl.storage-duration` | foundation |
| `decl.linkage` | planned |
| `decl.storage-specifiers` | planned |
| `decl.initializers` | foundation |
| `decl.designated-initializers` | foundation |
| `decl.empty-initialization` | planned |
| `decl.static-assert` | planned |
| `decl.attributes` | planned |

`Declare` now introduces native scalar (including complete enumerations), pointer, fixed-array and complete tagged-aggregate objects independently of assignment. Direct top-level declarations use static storage duration in the generated translation unit; block/procedure declarations use automatic storage duration. Scalar/pointer assignment-style initializers plus positional aggregate, named member-designated, and array index-designated initialization are type-checked. Omitted aggregate slots are zeroed. User-controlled linkage, `static`/`extern`/thread storage, allocated storage, nested aggregate initializers and full C constant-initializer rules remain missing.

## Statements and control flow

| ID | Status |
|---|---|
| `control.if` | implemented |
| `control.while` | implemented |
| `control.do-while` | planned |
| `control.for` | planned |
| `control.switch` | planned |
| `control.break` | planned |
| `control.continue` | planned |
| `control.goto-labels` | planned |
| `control.return` | foundation |

PlainSpeak's `Repeat` and `For each` remain useful language extensions, but they are not counted as replacements for every general C `for` loop.

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

Typed Procedures now have explicit native parameter and return types, recursive native qualifiers, checked calls, C array-parameter adjustment, generated prototypes, forward calls and mutual recursion. Typed `void` and value returns are checked. These rows remain **foundation** because variadic definitions/calls, function pointers, C's full compatible-type/prototype rules and complete path-sensitive return analysis are not finished.

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
