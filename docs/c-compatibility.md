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
| `types.complex` | foundation | Complex decimal native objects lower to double _Complex, can be constructed with `Complex with real A and imaginary B` (the CMPLX compiler builtin), support +, -, * and / and the real/imaginary/magnitude/conjugate queries, and reject invalid relational ordering; float/long-double complex variants and complete conversion rules remain pending. |
| `types.boolean` | foundation | Native `_Bool` objects are spellable; legacy true/false literals still preserve numeric compatibility. |
| `types.nullptr` | foundation | C23 `null pointer` and `null pointer type` are source-spellable; the distinct scalar type has void-pointer/character-pointer-compatible layout, C23 default initialization, native object storage, and supported null-constant/pointer/bool conversions enforced semantically. |
| `types.object-representation` | foundation | Explicit scalar, pointer, fixed-array, tagged-structure, tagged-union and tagged-enumeration objects use real C storage, address, size and target layout; padding/effective-type rules and complete lifetime semantics remain pending. |
| `types.sizeof-alignof` | foundation | Type queries plus object-expression `Size of` preserve fixed-array extent and complete structure/union/enum layout as well as scalar/pointer layout; requested alignment and native `size_t` remain pending. |
| `types.qualifiers` | foundation | Recursive `constant`/`volatile`/`restricted`/`atomic` source qualifiers lower to native C const/volatile/restrict/_Atomic, preserve pointer placement, enforce const modifiability and directional pointee qualification; full typedef/array compatibility details and constant-initializer lowering remain pending. |
| `types.pointers` | foundation | Recursive object pointers support address/dereference, array decay, element-scaled +/- arithmetic, pointer difference/comparison, +=/-= offsets, pointer-level qualifiers and qualifier-adding pointee conversions. Parameterized and variadic function-pointer types, function addresses, value calls, and void statement calls are source-spellable; null function-pointer conversions and complete compatibility rules remain pending. |
| `types.function-types` | implemented | Explicit typed Procedure parameters/returns lower to native C function types with recursive native qualifiers, C array-parameter adjustment, checked calls, generated prototypes, forward calls and mutual recursion. Variadics, function pointers and complete compatible-type rules remain pending in their own rows. |
| `types.arrays` | foundation | Positive fixed-bound native arrays are source-spellable with C storage, sizeof, subscript/store and ordinary array-to-pointer decay; incomplete source declarations and whole-array initialization remain pending. |
| `types.vla` | foundation | Block-scope variable-length native arrays accept native integral bounds, lower to C99 VLAs, support subscripting and runtime sizeof, and reject file-scope, aggregate-member, invalid-bound, and initializer cases; variably modified pointer/function shapes remain pending. |
| `types.structures` | implemented | Tagged structures have native layout/member access, named/unnamed bit-fields, C99 flexible-array tails and C11 anonymous structure/union members (Anonymous field as structure TAG.) whose members are reached directly, with scalar-typed and name-conflicting anonymous members rejected. |
| `types.bit-fields` | foundation | Named/unnamed native C bit-fields support width checks, width-0 unnamed separators, member access/store and initialization, with completed enums required before layout; C23 _BitInt source types and exhaustive implementation-defined base/enum-representation detection remain pending. |
| `types.flexible-array-members` | implemented | Trailing C99 flexible structure members use real incomplete-array layout with sizeof, last-member, initializer and recursive union-containment restrictions, and extended objects can be allocated with malloc of the structure size plus element storage and then indexed through the flexible member. |
| `types.unions` | implemented | Tagged unions have native layout/member access, bit-fields and C11 anonymous members; a union cannot directly declare a flexible array member but may contain a flexible-array structure and then inherits C's structure-member/array-element restriction. |
| `types.enumerations` | foundation | Tagged enumerations have source definitions, implicit/explicit int-range enumerators, native enum storage, qualified enumerator expressions and typed transport; general integer constant expressions and C23 fixed underlying/wider rules remain pending. |
| `types.aliases` | implemented | Named aliases resolve to the structural native type model. |
| `types.typeof` | foundation | C23 `typeof` / `typeof_unqual` capability for native object names and parenthesized expressions. |
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
| `expr.subscript-member` | implemented | Native subscript/member reads/stores preserve effective const/volatile qualification, bit-fields and flexible-array typing, reach members of anonymous structure/union members directly, and reject atomic aggregate member access as C undefined behavior. |
| `expr.casts` | foundation |
| `expr.conditional` | foundation |
| `expr.sequencing` | foundation |
| `expr.function-calls` | foundation |
| `expr.compound-literals` | foundation |
| `expr.generic-selection` | foundation |
| `expr.nullptr-conversions` | foundation |

Explicit native objects model C modifiable-lvalue constraints: const-qualified objects and aggregates containing const subobjects cannot be mutated; pointer dereference, fixed-array elements and structure/union members (including named bit-fields) preserve effective const/volatile qualification. Arrays decay to element pointers in ordinary value contexts but retain extent for `Size of` and `Address of`. Pointer +/- integer, same-element-type pointer difference/comparison and pointer +=/-= offsets are supported. This remains **foundation** because function decay, null pointers, complete conversions, anonymous members, sequencing and the full usual arithmetic conversions are not complete.

PlainSpeak now classifies and evaluates a source-spellable subset of C integer constant expressions at translation time: integer/boolean constants, unary integer operators, integer binary arithmetic/bitwise/shift/comparison/logical operators and conditional expressions, including short-circuit unevaluated branches. This powers zero-valued null pointer constants without treating runtime zero values as constants. The row remains **foundation** until enumerator references, constant `sizeof`/`alignof`, integer casts, C23 `constexpr` names and the remaining extended rules are represented.

Native arithmetic expressions now apply C integer promotions and usual arithmetic conversions across the ordinary integer and real-floating families, and lower directly to C operators. Bitwise AND/XOR/OR/complement and shifts are source-spellable with promoted result types. This remains **foundation** because C23 `_BitInt` conversion rank interactions, complex arithmetic, full constant-expression overflow analysis, and exhaustive undefined/implementation-defined shift behavior are not yet covered.

Compound values now lower scalar, positional, member-designated and array-element-designated forms to native C99 compound literals. Their complete native object type and lvalue status are retained through semantic analysis; nested initializer lists and the remaining lifetime/constant-expression edge cases remain pending.

Type-directed selections now lower to C11 `_Generic`: every association has a complete non-duplicate type, an optional `Otherwise` expression supplies the C default association, and the selected branch retains its native or legacy expression type. The remaining qualifier-compatibility and exhaustive constraint rules remain pending.

Conditional expressions are now source-spellable and lower directly to C `?:`. Arithmetic branches use usual arithmetic conversions; compatible object-pointer branches compose pointed-to qualifiers and `void *`; identical structure/union branches are transported by value. This remains **foundation** because integer null-pointer constants, function pointers, void-valued branches and the remaining exhaustive composite-type rules are pending.

Obvious unsequenced native side effects are diagnosed before lowering: a binary expression cannot read or modify the same native object on both unsequenced sides. `and` and `or` retain their C short-circuit sequencing. This is a foundation for the complete C sequencing model; aliasing through arbitrary pointers and every C23 sequencing edge case remain pending.

Prefix/postfix increment and decrement are now source-spellable over native modifiable arithmetic/pointer lvalues, including array elements, dereferences, named bit-fields and C11 atomic scalar objects. The operators lower directly to C, preserving prefix/postfix value timing and pointer scaling. This remains **foundation** because function-pointer operands are not yet source-spellable and the repository has not yet completed the broader C sequencing model.

Zero-valued integer constant expressions from the current compile-time evaluator and C23 `nullptr_t` values now participate in object-pointer initialization/assignment, typed calls/returns, equality, scalar conditions and conditional expressions. C23 `nullptr_t` remains a distinct semantic scalar type and lowers through a C11-compatible pointer-sized backend representation. This remains **foundation** because the remaining integer constant-expression forms and function-pointer null conversions await their respective tranches.

Explicit scalar conversions are now source-spellable and lower to native C casts across arithmetic↔arithmetic, object-pointer↔object-pointer, integer↔pointer and scalar→boolean categories. C23 null-pointer constants may also explicitly convert to `null pointer type`, while `nullptr_t` converts to supported object pointers and boolean. This remains **foundation** because `Discard EXPR.` (a cast to void) now evaluates and discards an expression; function pointers, general zero-valued integer constant-expression recognition, and exhaustive implementation-defined pointer/integer guarantees are still pending.

## Declarations, storage and linkage

| ID | Status |
|---|---|
| `decl.explicit-declarations` | foundation |
| `decl.storage-duration` | foundation |
| `decl.linkage` | implemented | Native objects and Procedures can request internal (static) or external linkage; external objects declared without an initializer become extern declarations that resolve across separately compiled translation units, and conflicting Procedure redeclarations are rejected. |
| `decl.storage-specifiers` | foundation | Plain-English `static storage`, `internal linkage`, and `external linkage` clauses lower to C specifiers alongside thread-local storage; inline and full interaction rules remain pending. |
| `decl.initializers` | foundation |
| `decl.designated-initializers` | foundation |
| `decl.empty-initialization` | foundation | Native declarations of scalar, pointer, fixed-array, structure, union, enumeration and decimal objects with no initializer clause, or with the plain-English `with empty braces` clause, are zero-initialized in the generated C: integer/boolean/enumeration objects get 0, pointers get the platform null, decimal objects get 0.0, fixed-array elements and aggregate members are recursively zero-initialized. Legacy boxed declarations and the remaining C23 initializer-context rules remain pending. |
| `decl.static-assert` | implemented | `Assert that` lowers to C11 `_Static_assert` after integer-constant-expression validation, with an optional C11 diagnostic/C23 optional message. |
| `decl.attributes` | foundation | C23 deprecated, maybe-unused and nodiscard attributes are source-spellable (nodiscard on typed non-void Procedures, with an optional message). Deprecated uses emit E0035 and a discarded nodiscard result emits E0038 warnings; Discard EXPR. suppresses it. maybe-unused is retained in the semantic model with no runtime effect. Target-specific attribute mapping and the remaining C23 attribute grammar (fallthrough, reproducible, unsequenced, namespaced attributes) remain pending. |

`Declare` now introduces native scalar (including complete enumerations), pointer, fixed-array and complete tagged-aggregate objects independently of assignment. Direct top-level declarations use static storage duration in the generated translation unit; block/procedure declarations use automatic storage duration. Scalar/pointer assignment-style initializers plus positional aggregate, named member-designated, and array index-designated initialization are type-checked. Omitted aggregate slots are zeroed. Native declarations with no initializer at all are also zero-initialized (C23-style omitted-initializer behaviour for the native subset); user-controlled linkage, `static`/`extern`/thread storage, allocated storage, the C23 `{}` empty-brace spelling and full C constant-initializer rules remain missing.

Declaration attributes are represented structurally and checked before lowering. `deprecated` accepts an optional message and warns on native-object reads and calls to marked Procedures; `maybe unused` is accepted as a portable source annotation. This is foundation coverage only: target-specific attributes, attribute placement on every C declaration form, and the complete C23 attribute-token grammar remain pending.

`Assert that expression.` accepts an integer constant expression and rejects a zero result during semantic analysis. `with message "text"` supplies the diagnostic text emitted by the generated C11 `_Static_assert`; omitting it uses a deterministic PlainSpeak message. Failed, non-constant, and non-integral conditions use E0037.

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
| `func.prototypes` | implemented | Sema pre-registers signatures and generated C emits prototypes before definitions, enabling forward/mutual calls; Procedure NAME takes ... returns T defined elsewhere. gives a declaration-only prototype (compatible later definitions are accepted, conflicting ones rejected) that links across translation units. |
| `func.variadic` | foundation |
| `func.recursion` | implemented | Pre-registered signatures plus generated prototypes (and declaration-only Procedures defined elsewhere) support direct, forward, mutual and cross-unit recursive Procedures. |
| `func.inline` | implemented | with inline lowers to C99 inline: externally visible inline Procedures also get a non-inline declaration so an external definition exists (callable through pointers and from other units), and combined with internal linkage they become static inline. |
| `func.noreturn` | implemented | with no return lowers to the C11 _Noreturn specifier (the C23 [[noreturn]] spelling has identical semantics) for explicit void Procedures, rejects Return statements and reachable ends inside them, and treats calls to no-return Procedures and to exit/abort/_Exit/quick_exit/longjmp/thrd_exit as ending control flow so typed Procedures need no unreachable Return. |

Native pointers deliberately do not pass through legacy untyped `Procedure` parameters or returns yet; typed signatures/function pointers are the next required function-model layer. `with inline` and `with no return` preserve the corresponding C function specifiers, with no-return Procedures restricted to explicit `void` functions whose control flow cannot reach the end and which contain no `Return`; complete C inline/linkage compatibility rules remain pending.

Typed Procedures now have explicit native parameter and return types, recursive native qualifiers, checked calls, C array-parameter adjustment, generated prototypes, forward calls and mutual recursion. Typed `void` and value returns are checked. Variadic typed Procedures can initialize, consume and finish one implicit `va_list`, and can create, consume, and finish copied cursors through `Start variadic arguments`, `Next variadic argument as`, `Copy variadic arguments`, and `Finish variadic arguments`. `func.typed-signatures`, `func.prototypes`, `func.recursion` and `types.function-types` are **implemented** because the C99 function-type surface used by PlainSpeak is complete and end-to-end tested; variadic default promotions, function pointers, C's full compatible-type/prototype rules and complete path-sensitive return analysis remain pending in their own rows.

## Translation and preprocessing capability

PlainSpeak does not need to copy C's token-oriented preprocessor syntax, but it must provide equivalent compile-time/program-building capability where C programs depend on it.

| ID | Status |
|---|---|
| `pp.translation-units` | implemented | Several PlainSpeak sources compile as separate translation units in one command (entry unit first, then library units holding Procedures, types, imports and literal-initialized objects) and link into one program; Procedure NAME takes ... returns T defined elsewhere. declares a cross-unit Procedure, objects are shared with external linkage, shared declarations use Include, and conflicting redeclarations are rejected. |
| `pp.header-interop` | foundation |
| `pp.conditional-compilation` | implemented | Compile if / Elif accept the simple predicates (NAME is defined, not defined, equal to, at least ...) or a full compile-time expression over --define/predefined macro values with arithmetic, shifts, bitwise, comparison (including at least/at most), and/or/not, conditional and NAME is defined (an undefined name is 0, like #if); only the selected branch is analysed and lowered. Non-evaluable conditions are rejected (E0040). |
| `pp.macros` | implemented | Typed imports of object-like C constants expose header macros with original spelling, and PlainSpeak token macros (Define the macro NAME taking a and b: ... End macro.) provide object-like and function-like macros with one-token or parenthesised-group arguments, Expand (statement) and Substitute (expression) uses, nested expansion, stringize and paste operators and Undefine the macro. |
| `pp.variadic-vaopt` | implemented | Macros accept a trailing variadic parameter that receives every remaining argument (single tokens or parenthesised groups), and a Variadic option rest: ... End variadic option. region is kept only when variadic arguments were supplied, matching C23 __VA_OPT__. |
| `pp.elifdef` | implemented | Zero or more Elif NAME is defined / is not defined / is equal to N and the other value comparisons between Compile if and Otherwise/End compile if are checked in first-match-wins order before semantic analysis (covering #elifdef and #elifndef); the selected branch is analysed and lowered while all others are discarded. |
| `pp.warning` | implemented | Warning/Warn sentences produce non-fatal deterministic E0033 warnings and Error "message". is a fatal E0039 diagnostic (the C #warning and #error directives); both are frontend-only, emit no C, and fire only in the compile-time-selected branch. |
| `pp.embed` | implemented | Embed the file PATH as NAME reads a file (up to 1 MiB, relative to the source file) at compile time into a native array, with the C23 #embed parameters limit N, prefix and suffix byte lists, if empty fallback bytes, and an elements-of type for non-byte arrays; empty files without an if empty clause are rejected. |
| `pp.predefined-environment` | implemented | The compiler seeds a deterministic predefined environment — PLAINSPEAK, __STDC__, __STDC_HOSTED__, __STDC_VERSION__ (202311), __STDC_UTF_16__/__STDC_UTF_32__, __CHAR_BIT__, the __SIZEOF_*__ widths, __LP64__ and the host OS/architecture markers (__linux__, __APPLE__, _WIN32, __unix__, __x86_64__, __aarch64__) — that Compile if conditions can test numerically and --define can override. |
| `pp.pragma` | implemented | Pragma DIRECTIVE. lowers to a C #pragma line after validating the text (letters, digits, spaces and _ ( ) , . + - =); standard STDC pragmas (FP_CONTRACT, FENV_ACCESS, CX_LIMITED_RANGE with ON, OFF or DEFAULT) are validated, and other pragma names pass through to the target compiler as implementation-defined, as in C. |

`Import the C header`, `Import the C library`, typed external C-function declarations, and typed C-constant imports provide a deterministic interop foundation: generated C includes validated headers, the native link command receives validated libraries, imported calls retain native C signatures and symbol names, and object-like macros retain their case-sensitive C spelling. Full header parsing, function-like/variadic macro expansion, and separate translation units remain pending.

## Concurrency and C memory model

| ID | Status |
|---|---|
| `concurrency.atomics` | implemented | Native C11 _Atomic scalar/pointer objects support atomic load, store, exchange, compare-exchange (strong, with an expected-value object) and fetch add/subtract/and/or/xor, increment/decrement RMW, explicit memory orders on every operation and flag-style test-and-set through atomic booleans; invalid orders and mismatched types are rejected. |
| `concurrency.fences` | implemented | Atomic fence and Atomic signal fence lower to atomic_thread_fence and atomic_signal_fence with C11 seq_cst by default or an explicit relaxed/acquire/release/acquire release/sequentially consistent order clause. |
| `concurrency.lock-free` | implemented | Named atomic objects (Is lock free NAME) and types (Is lock free type T) can be queried through C11 atomic_is_lock_free and the compiler lock-free builtin. |
| `concurrency.threads` | implemented | Typed C imports with function-pointer parameters cover the C11 <threads.h> thread lifecycle: thrd_create, thrd_join with the start routine result, thrd_detach, thrd_exit (a no-return import), thrd_current/thrd_equal and sleeping through the runtime ps_sleep_ms helper; threads are plain unsigned-long handles. |
| `concurrency.thread-local` | implemented | Native declarations can request _Thread_local storage: file-scope thread-local objects are statically initialised from constant initializers so every thread starts with its own initial copy, block-scope thread-local objects require static storage, and non-constant initializers are rejected (E0042); threaded coverage checks per-thread values. |
| `concurrency.sync` | implemented | The runtime exposes C11 mutex (plain, recursive and timed), condition-variable (including timed waits) and call-once handles over <threads.h> that programs bind with typed imports from plainspeak_runtime.h and use with thrd_create/thrd_join. |
| `concurrency.memory-model` | implemented | PlainSpeak adopts the C11 memory model (documented in docs/c-compatibility.md): atomic operations default to sequentially consistent with explicit relaxed/acquire/release/acq_rel orders available, thread create/join and release-acquire pairs establish happens-before (covered by a message-passing test), and unsynchronised conflicting non-atomic accesses are data races with undefined behaviour as in C. |

Native `atomic` objects currently lower to real C11 `_Atomic` objects. Ordinary reads, simple assignments and stores through atomic-qualified pointers therefore use the C compiler's native default atomic semantics. This is only a foundation: explicit memory-order selection, the atomic RMW/API families, fences, lock-free queries, thread-local storage, threads/synchronization, and full happens-before/data-race conformance remain pending.

## Hosted C library

Each header row ultimately expands into per-facility entries as bindings are implemented. A header is not considered complete because one or two functions happen to exist in the current runtime.

| ID | Status | C surface |
|---|---|---|
| `lib.assert` | implemented | Runtime `Assert` uses the conforming `<assert.h>` facility for scalar conditions. |
| `lib.complex` | foundation | Real, imaginary, magnitude and conjugate queries use the platform <complex.h> functions, construction uses __builtin_complex, and typed imports can bind functions such as csqrt over complex decimal values; the remaining complex library surface and complex float/long-double variants remain pending. |
| `lib.ctype` | planned | `<ctype.h>` |
| `lib.errno` | implemented | errno is a typed object import that programs can clear, assign and compare with typed ERANGE/EDOM constant imports after library calls such as strtol and sqrt. |
| `lib.fenv` | implemented | Typed imports bind fenv.h functions (fegetround, fesetround, feclearexcept, fetestexcept, feraiseexcept) and the FE_* constants, with Pragma "STDC FENV_ACCESS ON" available for the pragma interaction. |
| `lib.float` | implemented | Minimum/maximum queries bind to <float.h>, and every other <float.h> macro (DBL_EPSILON, FLT_RADIX, DBL_DIG, DBL_MANT_DIG, FLT_MAX_EXP, DECIMAL_DIG ...) is available as a typed constant import with its original spelling. |
| `lib.inttypes` | implemented | Typed C imports bind <inttypes.h> functions (imaxabs, strtoimax, strtoumax ...) and format macros such as PRId64 are available as typed constant imports; unsigned 64-bit results print exactly. |
| `lib.limits` | implemented | Minimum/maximum queries bind to <limits.h>/<float.h>, and every <limits.h> macro (INT_MAX, UINT_MAX, LLONG_MAX, CHAR_BIT, MB_LEN_MAX ...) is available as a typed constant import with its original spelling. |
| `lib.locale` | foundation | Typed C imports can bind `<locale.h>` functions such as `setlocale`, including native character-pointer and null-pointer arguments; locale categories, `localeconv`, locale object ownership and the remaining locale semantics remain pending. |
| `lib.math` | implemented | Core trigonometric, logarithmic, power, absolute, floor/ceil, exponential, classification and round/trunc operations are built in, and the rest of <math.h> (fmod, hypot, cbrt, copysign, fmax, lround, ldexp, atan2, erf, tgamma, expm1 and the float/long-double suffix variants) binds through typed imports over decimal, float and long decimal values. |
| `lib.setjmp` | implemented | Numbered jump points give setjmp/longjmp semantics: ps_jump_mark is setjmp on point n and ps_jump is longjmp, bound through typed imports from plainspeak_runtime.h. |
| `lib.signal` | implemented | Typed imports bind signal.h constants (SIGTERM, SIGUSR1 ...), signal() with a typed Procedure handler passed as a function pointer, and raise(); handlers can communicate through atomic objects. |
| `lib.stdalign` | implemented | Native alignment requests lower to C11 _Alignas and Alignment of type lowers to _Alignof, which are the standard facilities behind the <stdalign.h> alignas/alignof macros. |
| `lib.stdarg` | foundation | Typed variadic Procedures use `<stdarg.h>` `va_list`, `va_start`, `va_arg`, `va_copy`, and `va_end` through deterministic PlainSpeak operations; default argument promotions, `va_end` control-flow obligations and the remaining header surface remain pending. |
| `lib.stdatomic` | implemented | atomic_load, atomic_store, atomic_exchange, atomic_compare_exchange_strong, atomic_fetch_add/sub/and/or/xor, atomic_thread_fence, atomic_signal_fence and atomic_is_lock_free bindings use <stdatomic.h> with explicit memory orders; atomic_init is the declaration initializer, and atomic_flag test-and-set/clear are the exchange/store forms on atomic booleans. |
| `lib.stdbool` | foundation | `<stdbool.h>` / C23 boolean spellings |
| `lib.stddef` | foundation | Native `size type` and `difference type` declarations exist; remaining `<stddef.h>` types/macros and exact ABI bindings remain pending. |
| `lib.stdint` | implemented | Exact-width, least and fast integer declarations lower through <stdint.h> types, and the limit macros (INT32_MAX, UINT64_MAX, SIZE_MAX, INTPTR_MAX, INTMAX_MAX ...) are typed constant imports; unsigned long values above LONG_MAX print exactly. |
| `lib.stdio` | foundation | Typed imports cover fopen/fputs/fputc/fflush/fclose, variadic fprintf with explicitly converted arguments, fgets into native character arrays, remove and the stdout object with opaque native stream pointers; FILE typing, binary I/O and buffering controls remain pending. |
| `lib.stdlib` | foundation | abs/labs/llabs and strtol/strtoul/strtod are also covered by typed imports. Typed imports cover allocation plus `atoi`/`atol`/`atof`/`strtoll` string conversions; the remainder of the conversion, process, sorting and searching surface remains pending. |
| `lib.stdnoreturn` | implemented | with no return lowers to the C11 _Noreturn specifier (the facility behind the <stdnoreturn.h> noreturn macro), with imports of the header accepted. |
| `lib.string` | foundation | Typed C imports can bind string.h strlen, strcmp, strncmp, strcpy, strcat, strchr and memset with string-literal and native character-array decay to character pointers, including in-place mutation of native arrays; bounds checking and the remaining string surface remain pending. |
| `lib.tgmath` | foundation | The <tgmath.h> header can be imported and typed imports can bind its underlying real math functions; type-generic macro dispatch remains pending. |
| `lib.threads` | implemented | Typed C imports bind <threads.h> thread lifecycle functions (thrd_create/join/detach/exit/current/equal), and the runtime wraps mutexes, condition variables, call_once and sleeping as plain integer handles. |
| `lib.time` | foundation | Typed C imports can bind time.h time with a null output pointer, clock with no arguments and difftime over integer time values; calendar conversion (struct tm), formatting and clock-type aliases remain pending. |
| `lib.uchar` | foundation | Typed C imports can bind C11/C23 <uchar.h> conversion functions such as c16rtomb with character-pointer and opaque state-pointer arguments; char16_t/char32_t/char8_t types and UTF literals remain pending. |
| `lib.wchar` | foundation | Typed C imports can bind <wchar.h> conversion functions such as btowc and wctob with wint_t-compatible integers; wide strings, wide stream I/O and mbstate_t objects remain pending. |
| `lib.wctype` | foundation | Typed C imports can bind <wctype.h> classification functions such as iswalpha and iswdigit with wint_t-compatible integer arguments; wctype_t/wctrans_t handles and wide-character literals remain pending. |
| `lib.stdbit` | foundation | Typed C imports can bind C23 <stdbit.h> unsigned-integer bit-query functions such as stdc_leading_zeros_ui, stdc_count_ones_ui and stdc_bit_width_ui; the type-generic stdc_* macros and endian macros remain pending. |
| `lib.stdckdint` | foundation | Checked add/subtract/multiply A and B into NAME expressions lower to the compiler overflow builtins behind C23 ckd_add/ckd_sub/ckd_mul, store the wrapped result in a modifiable native integer object and yield the overflow flag; mixed-signedness operand-type rules and non-integer result types remain pending. |

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

## Memory model

PlainSpeak adopts the C11 memory model for generated programs. Every atomic operation (`Atomic load`, `Atomic exchange`, `Atomic fetch ...`, atomic stores and atomic fences) is sequentially consistent. A thread's start (`thrd_create`) happens-before the start routine, and a routine's completion happens-before the matching `thrd_join` returns. Two conflicting accesses to the same non-atomic object from different threads without such an ordering are a data race and have undefined behaviour, exactly as in C; PlainSpeak does not currently detect them. `tests/golden/c_memory_model.eng` checks that three threads incrementing one atomic object always total 3000.

## Memory orders

Atomic fences, loads, stores, exchanges and fetch operations accept a trailing `with relaxed order`, `with acquire order`, `with release order`, `with acquire release order` or `with sequentially consistent order` clause (for example `Atomic store 1 to flag with release order.`). Omitting it keeps C11's default sequentially consistent semantics. A load may not be `release` or `acquire release`, and a store may not be `acquire` or `acquire release`.

## Anonymous members

Inside a `Structure` or `Union`, `Anonymous field as structure inner.` (or `as union number.`) embeds that aggregate by value without naming the member. Its members are then accessed directly on the outer object — `Member x of s` — as in C11. The generated C keeps the embedded aggregate under a compiler-generated member name, so layout is identical to a named member. The embedded type must be a structure or union, and names reachable through anonymous members may not duplicate other members of the same aggregate.

## Nested initializers

Nested aggregate initialization is written by placing a compound value inside a member or element initializer, for example `Declare s as structure segment with members start as Compound value of type structure point with values 1 followed by 2 done followed by finish as Compound value of type structure point with values 3 followed by 4 done done.` and `Declare grid as array of structure point with length 2 with elements at 1 as Compound value of type structure point with values 7 followed by 8 done done.` The generated C uses C99 compound literals; `tests/golden/c_nested_initializers.eng` covers both shapes.
