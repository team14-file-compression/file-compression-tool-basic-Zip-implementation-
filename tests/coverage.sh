#!/usr/bin/env bash
# coverage.sh - Code (line/function) and branch coverage for src/
#
# Builds instrumented binaries with GCC --coverage, runs the whole test suite
# (unit tests, system validation, lossless check), then writes reports with gcovr.
#
# Requirements: g++ and gcovr  (install gcovr with:  pip install gcovr)
# Usage (from the project root):
#   bash tests/coverage.sh
# Output:
#   Documents/Test_Reports/coverage/index.html      (line + branch, per file, per line)
#   Documents/Test_Reports/coverage/summary.txt     (line coverage table)
#   Documents/Test_Reports/coverage/branches.txt    (branch coverage table)
#   Documents/Test_Reports/coverage/coverage.json   (machine-readable summary)

set -eu

CXX="${CXX:-g++}"
OUT="Documents/Test_Reports/coverage"
B="build-coverage"
FLAGS="-std=c++17 -O0 -g --coverage -fprofile-update=atomic -Iinclude"

command -v gcovr > /dev/null || { echo "gcovr not found: pip install gcovr" >&2; exit 1; }

rm -rf "$B" "$OUT"
mkdir -p "$B" "$OUT"

# Compile each source once into object files so both binaries share coverage data.
for f in src/*.cpp tests/*.cpp; do
    $CXX $FLAGS -c "$f" -o "$B/$(basename "${f%.cpp}").o"
done
LIB_OBJS="$B/codec.o $B/file_input.o $B/frequency.o $B/huffman.o"
$CXX --coverage $B/main.o $LIB_OBJS -o "$B/huffzip"
$CXX --coverage $LIB_OBJS $B/test_*.o -o "$B/run_tests"

echo "== Unit tests =="
( cd "$B" && ./run_tests > unit_tests.log 2>&1 ) && echo "unit tests: PASS" || { echo "unit tests: FAIL"; exit 1; }
echo "== System validation =="
bash tests/system_validation.sh "$B/huffzip" | tail -1
echo "== Lossless check =="
bash tests/lossless_check.sh "$B/huffzip" | tail -1

COMMON=(--root . --filter 'src/' --object-directory "$B"
        --exclude-throw-branches --exclude-unreachable-branches --exclude-noncode-lines)

gcovr "${COMMON[@]}" --txt "$OUT/summary.txt"
gcovr "${COMMON[@]}" --txt-metric branch --txt "$OUT/branches.txt"
gcovr "${COMMON[@]}" --json-summary-pretty --json-summary "$OUT/coverage.json"
gcovr "${COMMON[@]}" --html-details "$OUT/index.html" --html-title "Team 14 - File Compression Tool - Coverage"

echo
cat "$OUT/summary.txt"
echo
cat "$OUT/branches.txt"
rm -rf "$B"
