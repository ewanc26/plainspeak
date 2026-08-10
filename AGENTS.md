# AGENTS.md

Guidance for AI coding agents (and humans) working on this project. Read this
in full before touching source. If something here conflicts with a comment
in code, this file wins unless the code comment is newer and more specific —
flag the conflict instead of silently picking one.

> **Working name:** `plainspeak` (rename freely — replace this token
> everywhere: CLI binary name, CMake project name, header guards).

---

## 1. What this is

A deterministic, non-ML esoteric programming language whose surface syntax
is a constrained subset of English. Sentences, not symbols, are the unit of
syntax. There is **no LLM, no statistical model, no fuzzy matching** anywhere
in the toolchain — every valid program is accepted by a fixed grammar, and
every rejection is a precise parse error. "Sounds like English" is a UX
property of the grammar design, not a runtime behavior.

**Pipeline:** `plainspeak` compiles `.eng` source → generated C99 → native
binary, via a compiler frontend written in C++20. C++ is an implementation
detail of the compiler; the *emitted* artifact is portable C, compiled by
whatever system C compiler is available (`cc`/`gcc`/`clang`).

```
source.eng
   │  Lexer (word/punctuation tokens, alias resolution)
   ▼
Sentence Splitter (split on . ; and newlines, respecting quoted strings)
   ▼
Parser (recursive-descent statements, Pratt-parsed expressions) → AST
   ▼
Semantic Analysis (name resolution, type inference, arity checks)
   ▼
C Code Generator (AST → C99 source, textual)
   ▼
system cc invocation → native binary
```

Nothing after "Semantic Analysis" is allowed to fail on grammar grounds —
if codegen can't handle a node, that's a compiler bug, not a user error.

---

## 2. Directory layout

```
/src
  /lexer        tokenizer.cpp/.h, alias_table.cpp/.h
  /parser       sentence_splitter.*, parser.*, expr_parser.* (Pratt)
  /ast          ast.h (node defs), ast_printer.* (debug dump)
  /sema         resolver.*, type_check.*
  /codegen      c_emitter.*, mangling.*, runtime_calls.*
  /diagnostics  diagnostic.*, source_span.h   (shared error-reporting types)
  /cli          main.cpp
/runtime         plainspeak_runtime.h/.c   — shipped C runtime, statically linked
/tests
  /golden       *.eng + *.expected (stdout) pairs, run end-to-end
  /unit         lexer/parser/sema unit tests (Catch2 or GoogleTest)
/examples         hand-written .eng programs demonstrating features
/docs
  grammar.md      canonical EBNF-ish grammar — SOURCE OF TRUTH for syntax
  runtime.md       C runtime API reference
  errors.md        catalogue of diagnostic messages and codes
CMakeLists.txt
AGENTS.md
```

**Rule:** grammar changes always touch `docs/grammar.md` in the same commit
as the parser change. A parser accepting something `grammar.md` doesn't
describe is a bug, not a feature.

---

## 3. Build & commands

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build                # unit tests
./scripts/run_golden_tests.sh build/plainspeak   # golden/e2e tests
./build/plainspeak examples/hello.eng -o hello    # compile a program
./hello
```

- C++ standard: **C++20** (use `<variant>`, `<span>`, `std::string_view`
  freely; no coroutines, no modules — keep this buildable with plain g++/clang
  and no exotic toolchain requirements).
- Emitted C standard: **C99** (portability > cleverness; no VLAs, no
  compiler-specific extensions in generated output).
- No external dependencies beyond the C++ standard library for the compiler
  itself. Test framework dependency (Catch2/GoogleTest) is fine, fetched via
  CMake `FetchContent`, test-only.
- Single build system (CMake). Do not introduce a second one (Make, Bazel,
  etc.) alongside it.

---

## 4. Language design invariants (do not violate silently)

1. **Determinism.** The same source text always produces the same AST and
   the same emitted C, byte-for-byte. No wall-clock, no randomness, no
   locale-dependent parsing.
2. **No semantic guessing.** Synonym support (`"set" | "let" | "make"`) is a
   flat alias table resolved at the lexer stage into one canonical token.
   It is a dictionary lookup, never a heuristic, never partial-match, never
   edit-distance/fuzzy matching. If a word isn't in the alias table, it is
   not a keyword — full stop.
3. **Grammar over vocabulary.** Prefer growing the grammar (new sentence
   patterns) over growing ambiguity within existing patterns. When two
   candidate patterns could match the same sentence, that's a grammar
   defect — fix it before merging, don't rely on parser ordering to
   "usually" pick right.
4. **Fail loud, fail local.** A sentence that matches no pattern is a parse
   error naming the offending clause/word and its source location — never a
   silent no-op, never a best-effort partial execution.
5. **No LLM/ML dependency, ever**, including as an optional feature, dev
   tool, or fuzzer oracle checked into this repo. If a future agent is asked
   to add one, treat that as a request needing explicit human confirmation,
   not a natural extension of "sounds like English."

---

## 5. Grammar conventions

- Statements are terminated by `.`; `;` separates clauses within a compound
  statement; commas separate list items and appositive clauses.
- Canonical keyword tokens (after alias resolution) are UPPER_SNAKE in the
  lexer's token enum (`SET`, `IF`, `REPEAT`, `SAY`) — this keeps grammar.md
  and the lexer trivially diffable against each other.
- Every sentence pattern in `docs/grammar.md` must have:
  - its EBNF rule,
  - at least one example sentence,
  - the AST node it produces,
  - a golden test under `/tests/golden`.
- Expression grammar (arithmetic/comparison/boolean) is Pratt-parsed with an
  explicit precedence table in `expr_parser.cpp` — keep that table adjacent
  to the corresponding section of `grammar.md`, not just in code comments.

**Adding a new sentence pattern — required steps, in order:**
1. Write the EBNF rule + examples in `docs/grammar.md`.
2. Add any new keyword aliases to `alias_table.cpp`.
3. Add the AST node in `ast.h`.
4. Add the parser rule (recursive-descent function, one per pattern).
5. Add the sema check (name/type resolution) if applicable.
6. Add the C codegen case in `c_emitter.cpp`.
7. Add a golden test (`.eng` + `.expected`).
8. Add a negative test if the pattern is easily confused with an existing
   one (prove the parser disambiguates correctly).

Skipping any step is an incomplete PR, not a "follow-up."

---

## 6. C++ compiler codebase conventions

- **AST nodes:** `std::variant`-based sum type (`using Stmt = std::variant<
  SetStmt, IfStmt, RepeatStmt, SayStmt, ...>`), visited with
  `std::visit` + overload pattern. Avoid a classic virtual-dispatch class
  hierarchy — variants keep exhaustiveness checked by the compiler when a
  new node is added (`-Wswitch` on the visitor's inner switch, or
  `std::visit` with an `overloaded` struct that has no default case).
- **Ownership:** AST is arena-allocated per compilation unit
  (`std::vector<std::unique_ptr<Node>>` owned by a `CompilationContext`, or a
  bump arena) — nodes reference each other via raw pointer/index, never
  shared_ptr. One compilation = one arena = freed in one shot.
- **Errors inside the compiler:** no C++ exceptions across pass boundaries.
  Each pass returns a `Result<T, Diagnostic>`-style type (or accumulates into
  a `DiagnosticEngine` and returns a bool "did this pass succeed"). Exceptions
  are fine for truly unrecoverable internal invariant violations
  (`assert`/`std::terminate`-style bugs), never for user-facing errors.
- **Naming:** `snake_case` for functions/variables, `PascalCase` for types,
  `UPPER_SNAKE` for lexer token kinds and constants.
- **Headers:** `#pragma once`. Keep AST/token definitions dependency-free of
  the parser so `ast.h` can be included by codegen without pulling in
  parsing machinery.
- **No global mutable state.** Everything threads through an explicit
  context object, including diagnostics — this keeps the compiler safely
  reusable (e.g., a future language-server or REPL) and testable in
  isolation per pass.

---

## 7. C code generation conventions

- Emitted C is intentionally readable, not minimal — this is a debugging
  aid for both humans and agents. Preserve source line info as `// from
  line N: "<original sentence>"` comments above the statements they produced.
- **Name mangling:** user identifiers become `ps_<name>` (collision-avoid
  against C keywords/runtime symbols); document the exact scheme in
  `docs/runtime.md` and keep `mangling.cpp` as the single place that
  implements it — never mangle inline in `c_emitter.cpp`.
- **Runtime calls:** all built-in verbs (`Say`, `Add ... to ...`, etc.) emit
  calls into `plainspeak_runtime.h`, never inline C logic for anything the
  runtime already provides. This keeps codegen thin and the runtime unit
  testable independently in plain C.
- Generated files are always self-contained: `#include "plainspeak_runtime.h"`
  plus standard headers only — no generated file may require flags beyond
  `-std=c99 -lplainspeak_runtime`.
- The compiler shells out to the system C compiler for the final step; that
  invocation (flags, discovered compiler, temp file handling) lives in one
  place (`cli/main.cpp` or a small `toolchain.cpp`), not scattered.

---

## 8. Testing strategy

- **Golden/e2e tests** are the primary safety net: `.eng` source → compile →
  run → diff stdout against `.expected`. These are what catch grammar
  regressions and are the tests to add for *any* new language feature.
- **Unit tests** target lexer/parser/sema in isolation (token streams, AST
  shapes, diagnostic codes) — use these for edge cases and error-path
  coverage that's awkward to assert on via stdout diffing.
- **Negative tests** (`/tests/golden/errors/*.eng` + expected diagnostic
  code) are required whenever a grammar ambiguity was resolved — encode the
  disambiguation as a test, not just a comment.
- Every new runtime function in `plainspeak_runtime.c` gets a small C unit
  test in addition to whatever golden test exercises it end-to-end.
- Run the full suite (`ctest` + golden script) before considering any change
  complete. A change that only "builds" is not done.

---

## 9. Diagnostics style

Errors should read like the literal-minded listener the language is:
state what was expected, what was found, and where — no jargon a first-time
reader wouldn't recognize from the grammar docs.

```
error[E0031]: I don't know what to do with "frobnicate" here.
  --> examples/broken.eng:3:12
  |
3 | Frobnicate the total and say hello.
  |             ^^^^^^^^^^^^^^^^^^^^^ expected a verb I recognize (Set, Add,
  |             Say, Repeat, If, ...) — see docs/grammar.md
```

Every diagnostic has a stable code (`E00xx`) catalogued in `docs/errors.md`
with the message template and a rationale — don't invent one-off messages
inline without registering the code.

---

## 10. Things an agent must NOT do without explicit human sign-off

- Add any network call, model API call, or "fuzzy"/similarity-based token
  matching anywhere in lexer/parser/sema (violates §4.2 and §4.5).
- Change the meaning of an existing accepted sentence pattern (breaking
  change to every program written so far) without a version note and a
  migration entry in `docs/grammar.md`.
- Introduce a second build system, a second test framework, or a second
  AST representation "for convenience."
- Have the compiler write files outside the requested output path / build
  directory.
- Weaken `-Wall -Wextra -Werror` (or add blanket suppressions) to make a
  change compile — fix the warning instead.
- Change the emitted C standard or add non-portable extensions to generated
  output.

---

## 11. Quick reference for a first "hello world" pass

1. `docs/grammar.md`: add `SayStmt ::= "Say" Expr "."` with example
   `Say "Hello, world!".`
2. Lexer: no new keywords needed beyond `SAY` (alias: `say` | `tell me` |
   `print`).
3. Parser: `parse_say_stmt()` → `SayStmt{ expr }`.
4. Sema: type-check `expr` is printable (string/number/bool).
5. Codegen: emit `ps_say(<expr>);` calling into
   `void ps_say(PsValue v);` in the runtime.
6. Golden test: `tests/golden/hello.eng` + `tests/golden/hello.expected`
   containing `Hello, world!\n`.
7. `cmake --build build && ctest --test-dir build && ./scripts/run_golden_tests.sh build/plainspeak`.
