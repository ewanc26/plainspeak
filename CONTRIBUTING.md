# Contributing to plainspeak

A deterministic, non-ML prose-syntax systems programming language: programs are written as paragraphs, with punctuation and explicit phrases (`End if.`, `Set x to …`) carrying structure while line breaks are ordinary whitespace. The capability target is the union of **C99, C11, C17 and C23**, and the compiler frontend is C++20. Thanks for wanting to help!

## Getting started

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Start with `docs/grammar.md` (accepted syntax) and `docs/c-compatibility.md` (the C99–C23 capability matrix).

## Notes for this project

- **Determinism is the name and the rule.** No ML, no undocumented host-compiler behaviour: frontend semantics and conformance tests define the language, not accidental capabilities of whatever C compiler runs the output. If you rely on something the host compiler does but the spec doesn't specify, the PR is wrong.
- Parentheses at statement boundaries are comments; inside expressions they group arithmetic — this dual role is deliberate; don't "fix" it.
- Programs lower through readable portable C plus the PlainSpeak runtime; the host C compiler may be newer than C99, but lowering should not *depend* on host-specific extensions for C99-level features.
- Arbitrary embedded C does **not** count as supporting a capability: that's explicitly out of scope.
- Update the grammar doc and the capability matrix in the same PR as any change to either.

## Before opening a PR

- Build passes and conformance tests run; include conformance coverage for anything newly supported.
- Conventional commits; focused PRs.
- AI-assisted PRs welcome; stand behind every line and run the tests yourself.

## Licence

See [LICENSE](LICENSE). Contributions accepted under the same terms.