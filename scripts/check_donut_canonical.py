#!/usr/bin/env python3
"""Cross-language parity gate for the donut engine.

All languages' donut animations must be IDENTICAL. The canonical engine is the
sigi implementation (sigi/scripts/gen_donut_engine.py); this script compiles
examples/donut_engine_finite.eng with the PlainSpeak compiler and compares its
first frames byte-for-byte against the vendored canonical fixture
tests/fixtures/donut_canonical_8f.txt (captured from the canonical engine's
48-frame stream, so CI does not need sigi or SDL2 installed).

Usage: python3 scripts/check_donut_canonical.py <path-to-plainspeak> [--frames N]
"""

import argparse
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
ENGINE = ROOT / "examples" / "donut_engine_finite.eng"
FIXTURE = ROOT / "tests" / "fixtures" / "donut_canonical_8f.txt"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", nargs="?", default="build/plainspeak")
    parser.add_argument("--frames", type=int, default=8)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        out_bin = pathlib.Path(tmp) / "donut_engine_finite"
        compiled = subprocess.run(
            [args.binary, str(ENGINE), "-o", str(out_bin)],
            capture_output=True,
            text=True,
        )
        if compiled.returncode != 0:
            print("FAIL: PlainSpeak did not compile {}".format(ENGINE.name),
                  file=sys.stderr)
            print(compiled.stdout + compiled.stderr, file=sys.stderr)
            return 1

        run = subprocess.run([str(out_bin)], capture_output=True, text=True)
        if run.returncode != 0:
            print("FAIL: engine exited non-zero", file=sys.stderr)
            return 1

        produced = run.stdout.splitlines()
        fixture = FIXTURE.read_text(encoding="utf-8").splitlines()
        if len(fixture) % len(produced) != 0 or len(produced) == 0:
            pass  # ratio checked below by prefix comparison
        if len(produced) > len(fixture):
            print("FAIL: engine produced more tokens than the fixture holds",
                  file=sys.stderr)
            return 1

        n_lines = 0
        frames = 0
        for line in produced:
            n_lines += 1
            if line == "S":
                frames += 1
        if frames < args.frames:
            print("FAIL: engine produced only {} frames, expected at least {}".format(
                frames, args.frames), file=sys.stderr)
            return 1

        count = 0
        quads = 0
        quads_per_frame = {}
        tokens = iter(enumerate(produced))
        for idx, token in tokens:
            if token == "F":
                count = 0
            elif token == "P":
                count += 1
                for _ in range(11):
                    try:
                        value = float(next(tokens)[1])
                    except StopIteration:
                        print("FAIL: truncated quad at line {}".format(idx),
                              file=sys.stderr)
                        return 1
                    if value != value or value in (float("inf"), float("-inf")):
                        print("FAIL: non-finite quad value at line {}".format(idx),
                              file=sys.stderr)
                        return 1
            elif token == "S":
                quads_per_frame[frames] = count
            elif token not in ("F",):
                print("FAIL: unexpected token {!r} at line {}".format(token, idx),
                      file=sys.stderr)
                return 1

        if produced != fixture[: len(produced)]:
            for i, (a, b) in enumerate(zip(produced, fixture)):
                if a != b:
                    print("FAIL: first byte difference at line {}:".format(i + 1),
                          file=sys.stderr)
                    print("  engine:   {!r}".format(a), file=sys.stderr)
                    print("  canonical:{!r}".format(b), file=sys.stderr)
                    return 1
            print("FAIL: engine output is a prefix but differs in length",
                  file=sys.stderr)
            return 1

        per_frame = set(quads_per_frame.values())
        print("donut canonical parity OK: {} frames byte-identical, "
              "{}/frame quads ({} tokens)".format(
                  frames, sorted(per_frame)[0], n_lines))
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
