# plainspeak

A deterministic, non-ML esoteric programming language whose syntax is a constrained subset of English. PlainSpeak programs are written as prose paragraphs: punctuation and explicit phrases such as `End if.` carry the structure, while line breaks and indentation are just whitespace.

The compiler frontend is C++20, emits readable portable C99, then hands that C to the system compiler for the native binary.

```text
Set primes to List with 2 followed by 3 followed by 5 done. (Grow the list before reading it.) Append 7 to primes. For each prime in primes: Say prime. End for.
```

Parentheses at statement boundaries are comments. Parentheses inside expressions still group arithmetic, so `Say (2 plus 3) times 4.` behaves as expected.

See `AGENTS.md` for architecture/conventions and `docs/grammar.md` for the full syntax reference.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build
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
