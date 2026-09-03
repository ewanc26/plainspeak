#!/usr/bin/env python3
"""Validate the C99-C23 capability manifest used by CI.

The manifest is intentionally status-oriented rather than a claim of standards
conformance. It makes missing facilities explicit and prevents implemented or
foundation work from being marked without an in-repository regression test.
"""

from __future__ import annotations

import json
from collections import Counter
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "tests" / "conformance" / "c99-c23.json"
DOC = ROOT / "docs" / "c-compatibility.md"

ALLOWED_REVISIONS = {"C99", "C11", "C17", "C23"}
ALLOWED_AREAS = {"language", "preprocessor", "concurrency", "library"}
ALLOWED_STATUSES = {"planned", "foundation", "implemented", "non_applicable"}


def fail(message: str) -> None:
    print(f"conformance manifest error: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> None:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if data.get("schema") != 1:
        fail("schema must be 1")
    if data.get("target") != "C99-C23 capability parity":
        fail("unexpected target")

    features = data.get("features")
    if not isinstance(features, list) or not features:
        fail("features must be a non-empty list")

    ids: set[str] = set()
    counts: Counter[str] = Counter()
    doc_text = DOC.read_text(encoding="utf-8")

    for index, feature in enumerate(features):
        if not isinstance(feature, dict):
            fail(f"feature #{index + 1} is not an object")

        feature_id = feature.get("id")
        if not isinstance(feature_id, str) or not feature_id:
            fail(f"feature #{index + 1} has no id")
        if feature_id in ids:
            fail(f"duplicate feature id {feature_id}")
        ids.add(feature_id)

        revision = feature.get("revision")
        area = feature.get("area")
        status = feature.get("status")
        tests = feature.get("tests")
        note = feature.get("note")

        if revision not in ALLOWED_REVISIONS:
            fail(f"{feature_id}: invalid revision {revision!r}")
        if area not in ALLOWED_AREAS:
            fail(f"{feature_id}: invalid area {area!r}")
        if status not in ALLOWED_STATUSES:
            fail(f"{feature_id}: invalid status {status!r}")
        if not isinstance(tests, list) or not all(isinstance(t, str) and t for t in tests):
            fail(f"{feature_id}: tests must be a list of paths")
        if not isinstance(note, str) or not note:
            fail(f"{feature_id}: note is required")

        if status in {"foundation", "implemented"} and not tests:
            fail(f"{feature_id}: {status} features require at least one regression test")
        if status == "non_applicable" and "rationale" not in feature:
            fail(f"{feature_id}: non_applicable features require a rationale")

        for test_path in tests:
            path = ROOT / test_path
            if not path.exists():
                fail(f"{feature_id}: test path does not exist: {test_path}")

        if f"`{feature_id}`" not in doc_text:
            fail(f"{feature_id}: missing from docs/c-compatibility.md")

        counts[status] += 1

    print(
        "C99-C23 capability manifest OK: "
        + ", ".join(f"{status}={counts[status]}" for status in sorted(ALLOWED_STATUSES))
        + f", total={len(features)}"
    )


if __name__ == "__main__":
    main()
