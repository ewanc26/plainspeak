#!/usr/bin/env sh
# Usage: ./scripts/run_golden_tests.sh <path-to-plainspeak-binary>
# Compiles every tests/golden/*.eng, runs it, and diffs stdout against the
# matching .expected file. Exits non-zero on the first mismatch.
set -e

BIN="${1:-build/plainspeak}"
DIR="$(cd "$(dirname "$0")/.." && pwd)"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

fail=0
for src in "$DIR"/tests/golden/*.eng; do
    name="$(basename "$src" .eng)"
    expected="$DIR/tests/golden/$name.expected"
    [ -f "$expected" ] || { echo "SKIP $name (no .expected file)"; continue; }

    out_bin="$TMP/$name"
    if ! "$BIN" "$src" -o "$out_bin" > "$TMP/$name.compile.log" 2>&1; then
        echo "FAIL $name: did not compile"
        cat "$TMP/$name.compile.log"
        fail=1
        continue
    fi

    if ! "$out_bin" > "$TMP/$name.actual" 2>&1; then
        echo "FAIL $name: program exited non-zero"
        fail=1
        continue
    fi

    if diff -u "$expected" "$TMP/$name.actual" > "$TMP/$name.diff"; then
        echo "PASS $name"
    else
        echo "FAIL $name: output mismatch"
        cat "$TMP/$name.diff"
        fail=1
    fi
done

exit $fail
