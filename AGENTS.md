# AGENTS.md

Guidance for AI coding agents and humans working on PlainSpeak. Read this before changing the compiler. If code comments conflict with this file, flag the conflict instead of silently choosing one.

---

## 1. What PlainSpeak is

PlainSpeak is a deterministic, non-ML **prose-syntax systems programming language**. Its syntax is constrained English: sentences and paragraphs are the source form, but every accepted program is defined by a fixed grammar. There is no LLM, fuzzy matching, statistical parser, or semantic guessing in the toolchain.

The language's capability target is the union of the programming facilities in **C99, C11, C17 and C23**. That target includes the C object/type model, expressions and conversions, storage duration/linkage, translation-unit capability, preprocessing-equivalent compile-time facilities, atomics/threads, and the hosted/freestanding standard-library surface. PlainSpeak does not need to copy C token syntax, but it must be able to express equivalent program behaviour through first-class PlainSpeak syntax or typed standard bindings.

An arbitrary embedded-C escape hatch does **not** count as implementing a C capability.

Current pipeline:

```text
source.eng
   │
   ▼
Tokenizer (case-insensitive words, punctuation, exact aliases, parenthetical comments)
   │
   ▼
Recursive-descent parser → arena-owned variant AST
   │
   ▼
Semantic analysis (name resolution, structural types, diagnostics)
   │
   ▼
C backend + PlainSpeak runtime
   │
   ▼
system C compiler → native binary
```

Physical newlines and indentation in `.eng` source are ordinary whitespace. Punctuation and explicit phrases such as `End if.` carry structure.

Nothing after semantic analysis may reject a program on grammar/type grounds. If a semantically valid AST cannot be lowered, that is a compiler bug or an explicitly unsupported conformance item, not an opportunity for codegen to guess.

---

## 2. Source-of-truth documents

- `docs/grammar.md` — canonical accepted PlainSpeak syntax.
- `docs/c-compatibility.md` — human C99-C23 capability matrix and rules.
- `tests/conformance/c99-c23.json` — machine-readable capability status.
- `docs/runtime.md` — generated-code/runtime ABI contract.
- `docs/errors.md` — stable diagnostic catalogue.

The umbrella C parity work is tracked in the repository issue titled `epic: reach C99–C23 language and library capability parity`.

A parser accepting syntax not described by `docs/grammar.md` is a bug. A capability marked `foundation` or `implemented` in the conformance manifest without a real regression-test path is also a bug; CI enforces this.

---

## 3. Build and validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
python3 scripts/check_c_conformance.py
./scripts/run_golden_tests.sh build/plainspeak
```

Compiler implementation:

- C++20.
- CMake is the single build system.
- No compiler runtime dependency beyond the C/C++ standard environments and explicitly documented platform/toolchain facilities.
- Catch2 is test-only through CMake `FetchContent`.

Generated-code policy:

- Portable C99 is the **baseline lowering dialect**, not the language's semantic ceiling.
- Prefer lowering newer semantics through portable runtime helpers when that preserves the required behaviour.
- A feature may elevate generated code to a newer C dialect or use a target capability when C99 cannot faithfully provide the required semantics/ABI. Such changes need feature detection, documentation, and conformance coverage.
- Do not use compiler extensions merely for convenience. Implementation-defined target bindings are acceptable only when the C standard itself makes the behaviour implementation-defined and PlainSpeak exposes that fact deliberately.
- Current generated native programs link the PlainSpeak runtime and `libm` where required.

---

## 4. Language-design invariants

1. **Determinism.** Same source and selected target configuration → same AST and generated source. No wall-clock/random/locale-dependent parsing.
2. **No semantic guessing.** Synonyms are exact dictionary aliases resolved by the lexer. No fuzzy matching or probabilistic interpretation.
3. **Grammar over vocabulary.** Prefer a new unambiguous sentence pattern over stuffing more meanings into an existing one.
4. **Fail loud and local.** Invalid source produces a stable diagnostic at the offending construct; never silently ignore it.
5. **Prose is syntax, not formatting.** Newlines/indentation never define nesting. Sentences end with punctuation and compound statements use explicit closing phrases.
6. **C capability parity is measurable.** Missing facilities stay `planned`; partial representations stay `foundation`; only tested usable facilities become `implemented`.
7. **No arbitrary-C cheat.** Interop with C is required, but embedding untyped C snippets does not satisfy a conformance row.
8. **No LLM/ML dependency.** This applies to the shipped compiler and grammar behaviour.

---

## 5. C99-C23 conformance workflow

For a C capability change:

1. Identify or add the feature ID in `tests/conformance/c99-c23.json`.
2. Add/adjust the structural semantic representation first if the feature introduces a type, object property, storage rule or value category.
3. Design deterministic PlainSpeak syntax or a typed standard-library binding.
4. Update `docs/grammar.md` for syntax changes.
5. Update AST/parser/sema/codegen/runtime together as required.
6. Add positive end-to-end coverage and focused unit coverage.
7. Add negative diagnostics for invalid C-semantic cases.
8. Move manifest status only as far as the tests justify (`planned` → `foundation` → `implemented`).
9. Update `docs/c-compatibility.md` in the same change.
10. Run the entire validation stack above.

Never mark a header-level standard-library row implemented because a single function exists. Split rows into per-facility entries as implementation grows.

C17 is primarily a defect-fix revision, so its work usually appears as semantic/diagnostic corrections to C11 facilities rather than flashy new syntax. Still track those corrections explicitly where behaviour changes.

---

## 6. AST and semantic types

AST rules:

- AST nodes are plain structs collected in `std::variant` sum types.
- Nodes are arena-owned per compilation unit and referenced by stable raw pointers.
- Do not introduce a second AST hierarchy or virtual-dispatch tree.

Semantic type rules:

- `src/sema/type.h` is the structural type representation used for the C parity project.
- Do not add a new flat enum variant every time a C type arrives. C types are compositional: pointer-to-array-of-function/etc. must be representable structurally.
- Type identity must include all properties that affect C compatibility: integer rank/signedness, floating rank, qualifiers, bit-precise width, referenced element type, array bounds, function parameter/return/variadic shape, and aggregate/enum identity.
- Existing PlainSpeak `number` currently maps to signed C `long`; `decimal` currently maps to C `double`. Preserve existing program behaviour while the explicit typed-declaration surface is introduced.
- Lists are a PlainSpeak extension and remain homogeneous mutable reference values. They are not a substitute for C arrays/pointers.

---

## 7. Grammar conventions

- `.` terminates an ordinary sentence.
- `:` opens a compound sentence/block.
- Explicit `End <block>.` phrases close blocks.
- Commas are optional prose punctuation where the grammar declares them insignificant.
- Standalone parentheticals at statement boundaries are comments.
- Parentheses inside expressions group expressions.
- Newlines are whitespace only.

Every new sentence pattern needs:

1. EBNF-ish rule and example in `docs/grammar.md`.
2. Lexer aliases/tokens if needed.
3. AST node.
4. Parser rule.
5. Semantic checks.
6. Codegen/runtime lowering.
7. Golden test.
8. Negative test when ambiguity or invalid typing is plausible.
9. Conformance-manifest mapping when the feature implements C capability.

Skipping required layers makes the change incomplete.

---

## 8. C++ compiler conventions

- C++20 standard library is available; keep dependencies modest and portable.
- `snake_case` for functions/variables, `PascalCase` for types, canonical token spelling as established by the lexer.
- `#pragma once` headers.
- No global mutable compiler state.
- No exceptions for ordinary user-facing compiler errors across pass boundaries; return/accumulate diagnostics.
- Keep semantic decisions out of codegen. Codegen lowers already-validated AST/types.
- Preserve deterministic output ordering.

---

## 9. Code generation and runtime

- Generated C should remain readable and source-correlated.
- User identifiers are mangled only through `src/codegen/mangling.cpp`.
- Built-in runtime operations go through `plainspeak_runtime.h/.c` instead of duplicating runtime logic in emitted statements.
- Runtime helpers added for C capability work need plain-C unit tests plus a language-level test when exposed to PlainSpeak.
- C standard-library bindings should prefer the platform's conforming implementation where observable semantics/ABI matter, with a PlainSpeak type-safe wrapper rather than reimplementing libc casually.
- Pointer/atomic/concurrency work must be tested under sanitizers where practical once those features become executable.
- Separate translation-unit and C ABI interop tests are required before claiming function/object interoperability.

---

## 10. Testing strategy

- **Golden/e2e:** `.eng` → compile → run → expected output; primary language regression net.
- **Unit:** lexer/parser/sema/type/runtime edge cases.
- **Negative:** invalid programs and stable diagnostic codes.
- **Conformance manifest:** machine-checkable map of what is missing, foundational or implemented.
- **Cross-toolchain:** Linux and macOS CI remain required. C ABI/memory work should grow explicit GCC+Clang coverage rather than relying on only whichever `cc` happens to be default.
- **Sanitizers:** add ASan/UBSan/TSan coverage when addressable memory/concurrency reaches executable status.

A change that only compiles is not finished.

---

## 11. Diagnostics

Diagnostics should read like a literal-minded listener: say what PlainSpeak expected, what it found, and where. Keep stable error codes catalogued in `docs/errors.md`. Do not leak backend C diagnostics as the primary explanation for a frontend-invalid program.

---

## 12. Do not do these without explicit human sign-off

- Add network/model/fuzzy language behaviour to the compiler.
- Change the meaning of already accepted PlainSpeak syntax without a compatibility/migration note.
- Introduce a second build system, second test framework, or second AST representation for convenience.
- Weaken compiler warnings or blanket-suppress them to make a patch pass.
- Claim C parity by inserting arbitrary user C into generated output.
- Delete or downgrade conformance rows to make the percentage look better.

The user has explicitly signed off on evolving the backend beyond a C99-only ceiling **when necessary to achieve C99-C23 capability parity**. That permission does not waive portability, feature-detection, testing, or documentation requirements.
