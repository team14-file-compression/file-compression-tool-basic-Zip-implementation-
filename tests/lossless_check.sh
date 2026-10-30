#!/usr/bin/env bash
# lossless_check.sh - Issue #11 / SRS 6.4 "strictly lossless" / STP TC-DEC-03
#
# Compresses and decompresses a set of files with ./huffzip and checks that
# every restored file is byte-for-byte identical to the original (cmp).
# Also checks that damaged archives are rejected and leave no output file.
#
# Usage (from the project root, after `make`):
#   bash tests/lossless_check.sh            # uses ./huffzip
#   bash tests/lossless_check.sh ./huffzip.exe
#
# Exit code: 0 if every check passes, 1 otherwise.

set -u

HUFFZIP="${1:-./huffzip}"
if [ ! -x "$HUFFZIP" ] && [ -x "$HUFFZIP.exe" ]; then HUFFZIP="$HUFFZIP.exe"; fi
if [ ! -x "$HUFFZIP" ]; then
    echo "Error: $HUFFZIP not found. Run 'make' first." >&2
    exit 1
fi

WORK="lossless_work"
rm -rf "$WORK"
mkdir -p "$WORK"

pass=0
fail=0

ok()  { echo "[PASS] $1"; pass=$((pass + 1)); }
bad() { echo "[FAIL] $1"; fail=$((fail + 1)); }

# ---------- 1. Test data ----------
: > "$WORK/empty.bin"                                        # 0 bytes
printf 'a' > "$WORK/one_byte.txt"                            # 1 byte
printf 'ab' > "$WORK/two_symbols.txt"                        # 2 symbols
printf 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' > "$WORK/single_symbol.txt"
printf 'abracadabra' > "$WORK/abracadabra.txt"
printf 'line one\r\nline two\r\nline three\r\n' > "$WORK/windows_crlf.txt"
seq 1 200000 > "$WORK/numbers.txt"                           # ~1.3 MB text
head -c 500000 /dev/urandom > "$WORK/random.bin"             # incompressible
awk 'BEGIN { for (i = 0; i < 20; i++) for (b = 0; b < 256; b++) printf "%c", b }' \
    > "$WORK/all_bytes.bin"                                  # every byte value
for i in $(seq 1 3000); do
    echo "The quick brown fox jumps over the lazy dog. Huffman coding test line $i."
done > "$WORK/english.txt"                                   # ~230 KB text
cp README.md "$WORK/readme_copy.md"
cp src/codec.cpp "$WORK/source_code.cpp"

# ---------- 2. Round trips ----------
echo "== Round-trip tests (compress -> decompress -> cmp) =="
for f in "$WORK"/*; do
    name=$(basename "$f")
    case "$name" in *.huff|*.out) continue ;; esac
    if ! "$HUFFZIP" -c "$f" -o "$f.huff" > /dev/null 2>&1; then
        bad "$name: compression failed"; continue
    fi
    if ! "$HUFFZIP" -d "$f.huff" -o "$f.out" > /dev/null 2>&1; then
        bad "$name: decompression failed"; continue
    fi
    if cmp -s "$f" "$f.out"; then
        orig=$(wc -c < "$f" | tr -d ' ')
        comp=$(wc -c < "$f.huff" | tr -d ' ')
        ok "$name ($orig bytes -> $comp bytes -> identical)"
    else
        bad "$name: restored file differs from original"
    fi
done

# ---------- 3. Damaged archives must be rejected ----------
echo "== Corruption tests (must fail and leave no output) =="
good="$WORK/numbers.txt.huff"
size=$(wc -c < "$good" | tr -d ' ')

head -c $((size - 50)) "$good" > "$WORK/bad_truncated.huff"
head -c 100 "$good" > "$WORK/bad_short_header.huff"
{ cat "$good"; printf 'GARBAGE'; } > "$WORK/bad_trailing.huff"
printf 'this is not a huff archive at all, just some text......' > "$WORK/bad_not_archive.huff"
head -c 2064 "$WORK/single_symbol.txt.huff" > "$WORK/bad_single_no_payload.huff"

for b in "$WORK"/bad_*.huff; do
    name=$(basename "$b")
    out="$b.out"
    rm -f "$out"
    if "$HUFFZIP" -d "$b" -o "$out" > /dev/null 2>&1; then
        bad "$name was accepted"
    elif [ -e "$out" ] || [ -e "$out.tmp" ]; then
        bad "$name rejected but left an output file"
    else
        ok "$name rejected, no output written"
    fi
done

# ---------- 4. Summary ----------
echo "== Result: $pass passed, $fail failed =="
rm -rf "$WORK"
[ "$fail" -eq 0 ]
