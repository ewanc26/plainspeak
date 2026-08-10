# plainspeak

A deterministic, non-ML esoteric programming language whose syntax is a
constrained subset of English. Compiles to C99 via a C++20 frontend, then
hands off to the system C compiler for the native binary.

See `AGENTS.md` for architecture/conventions and `docs/grammar.md` for the
full syntax reference.

## Build (with CMake, once installed)

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./scripts/run_golden_tests.sh build/plainspeak
```

## Build (fallback, no CMake)

```sh
g++ -std=c++20 -Wall -Wextra \
  -DPLAINSPEAK_RUNTIME_C="\"$(pwd)/runtime/plainspeak_runtime.c\"" \
  -DPLAINSPEAK_RUNTIME_DIR="\"$(pwd)/runtime\"" \
  -Isrc \
  src/lexer/tokenizer.cpp src/parser/parser.cpp src/codegen/c_emitter.cpp src/cli/main.cpp \
  -o plainspeak
```

## Try it

```sh
./plainspeak examples/hello.eng -o hello && ./hello
```
