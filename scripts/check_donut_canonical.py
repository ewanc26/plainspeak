#!/usr/bin/env python3
"""Cross-language parity gate for the donut engine.

All languages' donut animations must be IDENTICAL. The canonical is
doughnut.c's renderer, and the sigi repo's generator (sigi/scripts/
gen_donut_engine.py) defines the reference vertex stream; this script
compiles examples/donut_engine_finite.eng with the PlainSpeak compiler and
compares its output byte-for-byte against the vendored canonical fixture
tests/fixtures/donut_canonical_v2_2f.txt (captured from the canonical
engine's first 2 frames, so CI does not need sigi or SDL2 installed).

Protocol per frame: token "F", then for each of 1500 vertices (ordered
i*30+j, i in 0..49, j in 0..29) four lines px py z intensity (one %g value
per line), then "S".

Usage: python3 scripts/check_donut_canonical.py <path-to-plainspeak> [--frames N]
"""

import argparse
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
ENGINE = ROOT / "examples" / "donut_engine_finite.eng"
FIXTURE = ROOT / "tests" / "fixtures" / "donut_canonical_v2_2f.txt"

U_STEPS, V_STEPS = 50, 30
N_VERTS = U_STEPS * V_STEPS


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", nargs="?", default="build/plainspeak")
    parser.add_argument("--frames", type=int, default=2)
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

        # Structural validation of the F / (1500 x px py z intensity) / S
        # protocol, independent of the byte comparison.
        frames = 0
        vals = 0
        expected = "F"
        vertices_per_frame = {}
        for idx, token in enumerate(produced):
            if expected == "F":
                if token != "F":
                    print("FAIL: expected F at line {}, got {!r}".format(
                        idx + 1, token), file=sys.stderr)
                    return 1
                vals = 0
                expected = "value"
            elif expected == "S":
                if token != "S":
                    print("FAIL: expected S at line {}, got {!r}".format(
                        idx + 1, token), file=sys.stderr)
                    return 1
                if vals != N_VERTS * 4:
                    print("FAIL: frame {} has {} values, expected {}".format(
                        frames, vals, N_VERTS * 4), file=sys.stderr)
                    return 1
                vertices_per_frame[frames] = vals // 4
                frames += 1
                expected = "F"
            else:
                try:
                    value = float(token)
                except ValueError:
                    print("FAIL: expected a number at line {}, got {!r}".format(
                        idx + 1, token), file=sys.stderr)
                    return 1
                if value != value or value in (float("inf"), float("-inf")):
                    print("FAIL: non-finite vertex value at line {}".format(
                        idx + 1), file=sys.stderr)
                    return 1
                vals += 1
                if vals == N_VERTS * 4:
                    expected = "S"

        if frames < args.frames:
            print("FAIL: engine produced {} frames, expected at least {}".format(
                frames, args.frames), file=sys.stderr)
            return 1

        if produced != fixture:
            for i, (a, b) in enumerate(zip(produced, fixture)):
                if a != b:
                    print("FAIL: first byte difference at line {}:".format(i + 1),
                          file=sys.stderr)
                    print("  engine:    {!r}".format(a), file=sys.stderr)
                    print("  canonical: {!r}".format(b), file=sys.stderr)
                    return 1
            print("FAIL: engine output is a prefix but differs in length",
                  file=sys.stderr)
            return 1

        print("donut canonical parity OK: {} frames byte-identical, "
              "{} vertices each ({} lines)".format(
                  frames, N_VERTS, len(produced)))
        return 0


if __name__ == "__main__":
    raise SystemExit(main())
