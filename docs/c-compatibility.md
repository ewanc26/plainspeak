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
| `types.integer-model` | foundation | Integer ranks and signedness; current `number` remains signed C `long`. |
| `types.bitint` | foundation | C23 bit-precise integer type is structurally representable. |
| `types.floating-model` | foundation | Float/double/long-double ranks are structurally representable. |
| `types.complex` | planned | Complex objects and arithmetic. |
| `types.boolean` | foundation | Boolean type can be represented; legacy literal semantics remain numeric for now. |
| `types.nullptr` | foundation | C23 null-pointer type can be represented. |
| `types.object-representation` | planned | Object size, padding, representation and lifetime rules. |
| `types.sizeof-alignof` | planned | Size/alignment queries and alignment requests. |
| `types.qualifiers` | foundation | const/volatile/restrict/atomic qualification is represented structurally. |
| `types.pointers` | foundation | Pointer types are represented; pointer expressions/lowering are next. |
| `types.function-types` | foundation | Return/parameter/variadic function shapes are represented. |
| `types.arrays` | foundation | Known-bound and incomplete array types are represented. |
| `types.vla` | planned | C99 variable-length and variably modified types. |
| `types.structures` | foundation | Tagged structure identity is represented; members/layout pending. |
| `types.unions` | foundation | Tagged union identity is represented; members/layout pending. |
| `types.enumerations` | foundation | Enum identity is represented; enumerators/underlying rules pending. |
| `types.aliases` | planned | typedef-equivalent aliases. |
| `types.typeof` | planned | C23 `typeof` / `typeof_unqual` capability. |
| `types.auto-inference` | planned | C23 inferred `auto` capability. |
| `types.constexpr` | planned | C23 constexpr object capability. |

## Expressions and conversions

| ID | Status |
|---|---|
| `expr.integer-promotions` | planned |
| `expr.value-categories` | planned |
| `expr.arithmetic` | foundation |
| `expr.bitwise` | planned |
| `expr.shifts` | planned |
| `expr.assignment` | foundation |
| `expr.increment-decrement` | planned |
| `expr.address-indirection` | planned |
| `expr.subscript-member` | planned |
| `expr.casts` | planned |
| `expr.conditional` | planned |
| `expr.sequencing` | planned |
| `expr.function-calls` | foundation |
| `expr.compound-literals` | planned |
| `expr.generic-selection` | planned |
| `expr.nullptr-conversions` | planned |

## Declarations, storage and linkage

| ID | Status |
|---|---|
| `decl.explicit-declarations` | planned |
| `decl.storage-duration` | planned |
| `decl.linkage` | planned |
| `decl.storage-specifiers` | planned |
| `decl.initializers` | planned |
| `decl.designated-initializers` | planned |
| `decl.empty-initialization` | planned |
| `decl.static-assert` | planned |
| `decl.attributes` | planned |

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
| `func.prototypes` | planned |
| `func.variadic` | foundation |
| `func.recursion` | foundation |
| `func.inline` | planned |
| `func.noreturn` | planned |

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
| `concurrency.atomics` | planned |
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

The first blocker is the semantic type/object model. The old compiler represented every program type with six enum values; it could not grow cleanly into pointers, arrays, functions, aggregates, qualifiers, atomics or C23 bit-precise integers. The structural model introduced alongside this document is therefore tranche zero. It intentionally preserves current language behaviour while making those future types representable.

Next milestones are addressable objects + pointers/arrays + `sizeof`/alignment, then explicit typed declarations/function signatures, then aggregates/initializers and the remaining C99 expression/control-flow model. C11/C17/C23 facilities build on that base rather than being implemented as disconnected runtime tricks.
