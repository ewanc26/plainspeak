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
    define_file="$DIR/tests/golden/$name.defines"
    units_file="$DIR/tests/golden/$name.units"
    extra_units=""
    if [ -f "$units_file" ]; then
        # One additional translation unit per line, relative to tests/golden.
        while IFS= read -r unit; do
            [ -n "$unit" ] && extra_units="$extra_units $DIR/tests/golden/$unit"
        done < "$units_file"
    fi
    compile_status=0
    if [ -f "$define_file" ]; then
        define_value="$(sed -n '1p' "$define_file")"
        "$BIN" "$src" $extra_units --define "$define_value" -o "$out_bin" > "$TMP/$name.compile.log" 2>&1 || compile_status=$?
    else
        "$BIN" "$src" $extra_units -o "$out_bin" > "$TMP/$name.compile.log" 2>&1 || compile_status=$?
    fi
    if [ "$compile_status" -ne 0 ]; then
        echo "FAIL $name: did not compile"
        cat "$TMP/$name.compile.log"
        fail=1
        continue
    fi

    if [ -f "$DIR/tests/golden/$name.input" ]; then
        if ! "$out_bin" < "$DIR/tests/golden/$name.input" > "$TMP/$name.actual" 2>&1; then
            echo "FAIL $name: program exited non-zero"
            fail=1
            continue
        fi
    else
        if ! "$out_bin" > "$TMP/$name.actual" 2>&1; then
            echo "FAIL $name: program exited non-zero"
            fail=1
            continue
        fi
    fi

    if diff -u "$expected" "$TMP/$name.actual" > "$TMP/$name.diff"; then
        echo "PASS $name"
    else
        echo "FAIL $name: output mismatch"
        cat "$TMP/$name.diff"
        fail=1
    fi
done

for src in "$DIR"/tests/golden/errors/*.eng; do
    [ -e "$src" ] || break
    name="$(basename "$src" .eng)"
    expected="$DIR/tests/golden/errors/$name.expected"
    [ -f "$expected" ] || { echo "SKIP $name (no .expected file)"; continue; }

    err_units=""
    if [ -f "$DIR/tests/golden/errors/$name.units" ]; then
        while IFS= read -r unit; do
            [ -n "$unit" ] && err_units="$err_units $DIR/tests/golden/errors/$unit"
        done < "$DIR/tests/golden/errors/$name.units"
    fi
    if "$BIN" "$src" $err_units -o "$TMP/$name" > "$TMP/$name.actual" 2>&1; then
        echo "FAIL $name: expected compile error but succeeded"
        fail=1
        continue
    fi

    if diff -u "$expected" "$TMP/$name.actual" > "$TMP/$name.diff"; then
        echo "PASS $name"
    else
        echo "FAIL $name: error message mismatch"
        cat "$TMP/$name.diff"
        fail=1
    fi
done

exit $fail
