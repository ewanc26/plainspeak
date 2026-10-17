#!/usr/bin/env python3
"""Differential tests: PlainSpeak versus equivalent C programs.

Usage: scripts/run_differential_tests.py <path-to-plainspeak-binary> [--cc CC]

Each case in tests/differential/cases.json names a PlainSpeak source (.eng)
and a C source (.c) that express the same operation. Both are compiled, run,
and their stdout and exit status are compared. Only features marked
"implemented" in tests/conformance/c99-c23.json may be cited, so partial or
planned features are never presented as passing parity tests.
"""

import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
MANIFEST = ROOT / "tests" / "differential" / "cases.json"
CONFORMANCE = ROOT / "tests" / "conformance" / "c99-c23.json"
C_FLAGS = ["-std=c23", "-Wall", "-Wextra", "-Werror", "-pedantic"]


def load_implemented_features():
    data = json.loads(CONFORMANCE.read_text())
    return {f["id"]: f["status"] for f in data["features"]}


def run(cmd, **kwargs):
    return subprocess.run(cmd, capture_output=True, text=True, timeout=30, **kwargs)


def check_case(case, bin_path, cc, tmp, statuses):
    name = case["name"]
    problems = []
    for feature in case["features"]:
        if feature not in statuses:
            problems.append(f"unknown conformance feature {feature}")
        elif statuses[feature] != "implemented":
            problems.append(f"feature {feature} is {statuses[feature]}, not implemented")
    if problems:
        return False, [f"{name}: " + p for p in problems]

    eng = ROOT / "tests" / "differential" / f"{name}.eng"
    csrc = ROOT / "tests" / "differential" / f"{name}.c"
    ps_bin = pathlib.Path(tmp) / f"{name}_ps"
    c_bin = pathlib.Path(tmp) / f"{name}_c"

    compiled = run([str(bin_path), str(eng), "-o", str(ps_bin)])
    if compiled.returncode != 0:
        return False, [f"{name}: PlainSpeak did not compile {eng.name}", compiled.stdout + compiled.stderr]

    c_compiled = run([cc, *C_FLAGS, "-o", str(c_bin), str(csrc)])
    if c_compiled.returncode != 0:
        return False, [f"{name}: C reference did not compile {csrc.name}", c_compiled.stdout + c_compiled.stderr]

    ps_run = run([str(ps_bin)])
    c_run = run([str(c_bin)])
    if ps_run.stdout != c_run.stdout or ps_run.returncode != c_run.returncode:
        return False, [
            f"{name}: {eng.name} and {csrc.name} differ "
            f"(exit {ps_run.returncode} vs {c_run.returncode})",
            "--- PlainSpeak stdout\n" + ps_run.stdout,
            "--- C stdout\n" + c_run.stdout,
        ]
    return True, []


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()

    cases = json.loads(MANIFEST.read_text())["cases"]
    statuses = load_implemented_features()
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for case in cases:
            ok, messages = check_case(case, pathlib.Path(args.binary), args.cc, tmp, statuses)
            print(("PASS " if ok else "FAIL ") + case["name"])
            if not ok:
                failed += 1
                for m in messages:
                    print(m)
    print(f"{len(cases) - failed}/{len(cases)} differential cases passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
