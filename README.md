# plainspeak

A deterministic, non-ML **prose-syntax systems programming language**. PlainSpeak programs are written as paragraphs: punctuation and explicit phrases such as `End if.` carry structure, while line breaks and indentation are ordinary whitespace.

PlainSpeak's capability target is the union of **C99, C11, C17 and C23**: the language is intended to grow to cover the C object/type model, pointer and array semantics, aggregates, linkage/storage duration, preprocessing/build-time capabilities, atomics/threads, and the hosted/freestanding standard-library surface while retaining PlainSpeak syntax. Arbitrary embedded C does not count as support.

The compiler frontend is C++20. Current programs lower through readable portable C plus the PlainSpeak runtime and then use the system C compiler for the native binary. Backend lowering may use runtime helpers or a newer C dialect as C99-C23 facilities are implemented; the frontend semantics and conformance tests define the language, not accidental capabilities of the host compiler.

```text
Set primes to List with 2 followed by 3 followed by 5 done. (Grow the list before reading it.) Append 7 to primes. For each prime in primes: Say prime. End for.
```

Parentheses at statement boundaries are comments. Parentheses inside expressions still group arithmetic, so `Say (2 plus 3) times 4.` behaves as expected.

See:

- `docs/grammar.md` for the accepted PlainSpeak syntax.
- `docs/c-compatibility.md` for the C99-C23 capability matrix and conformance rules.
- `tests/conformance/c99-c23.json` for the machine-readable implementation status.
- `AGENTS.md` for compiler architecture and contribution invariants.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build
python3 scripts/check_c_conformance.py
./scripts/run_golden_tests.sh build/plainspeak
```

## Fallback build without CMake

```sh
g++ -std=c++20 -Wall -Wextra \
  -DPLAINSPEAK_RUNTIME_C="\"$(pwd)/runtime/plainspeak_runtime.c\"" \
  -DPLAINSPEAK_RUNTIME_DIR="\"$(pwd)/runtime\"" \
  -Isrc \
  src/lexer/tokenizer.cpp src/parser/parser.cpp src/sema/sema.cpp src/codegen/mangling.cpp src/codegen/c_emitter.cpp src/ast/ast_printer.cpp src/cli/main.cpp \
  -o plainspeak
```

## Try it

```sh
./plainspeak examples/hello.eng -o hello && ./hello
```

Inspect or lint a program without building a binary:

```sh
./plainspeak examples/hello.eng --lint
./plainspeak examples/hello.eng --show-generated-c
```

PlainSpeak source highlighting for VS Code is provided in
`editors/vscode-plainspeak`.
