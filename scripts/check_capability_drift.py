#!/usr/bin/env python3
"""Fail when plainspeak's shared matrix cells drift from its conformance manifest.

Every test a plainspeak cell marked implemented cites must be a differential case,
and every manifest feature that case covers must itself be implemented.
"""

import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MATRIX = ROOT / "capabilities.json"
MANIFEST = ROOT / "tests" / "conformance" / "c99-c23.json"
CASES = ROOT / "tests" / "differential" / "cases.json"


def main() -> None:
    matrix = json.loads(MATRIX.read_text(encoding="utf-8"))
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    cases = json.loads(CASES.read_text(encoding="utf-8"))["cases"]

    feature_status = {f["id"]: f["status"] for f in manifest["features"]}
    case_by_test = {f"tests/differential/{c['name']}.eng": c for c in cases}

    problems = []
    for row in matrix["rows"]:
        cell = row["cells"]["plainspeak"]
        if cell["status"] != "implemented":
            continue
        for test in cell["tests"]:
            case = case_by_test.get(test)
            if case is None:
                problems.append(f"{row['id']}: {test} is not a differential case")
                continue
            for feature in case["features"]:
                status = feature_status.get(feature)
                if status is None:
                    problems.append(f"{row['id']}: case {case['name']} names unknown feature {feature}")
                elif status != "implemented":
                    problems.append(
                        f"{row['id']}: case {case['name']} covers {feature}, which is {status} in the manifest"
                    )

    if problems:
        for problem in problems:
            print(f"capability drift: {problem}", file=sys.stderr)
        raise SystemExit(1)
    print("capability drift check OK")


if __name__ == "__main__":
    main()
