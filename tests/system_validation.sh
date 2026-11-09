#!/usr/bin/env bash
# system_validation.sh - System Validation tests (STP Section 13, Test Case Document v2.0)
#
# Runs every system-level test case against the built ./huffzip binary and
# prints one PASS/FAIL line per test case ID.
#
# Usage (from the project root, after `make`):
#   bash tests/system_validation.sh               # uses ./huffzip
#   bash tests/system_validation.sh ./huffzip.exe
#
# Exit code: 0 if every test case passes, 1 otherwise.

set -u

HZ="${1:-./huffzip}"
if [ ! -x "$HZ" ] && [ -x "$HZ.exe" ]; then HZ="$HZ.exe"; fi
if [ ! -x "$HZ" ]; then echo "Error: $HZ not found. Run 'make' first." >&2; exit 1; fi
case "$HZ" in /*) ;; *) HZ="$(pwd)/$HZ" ;; esac

W="sysval_work"
rm -rf "$W"; mkdir -p "$W"; cd "$W" || exit 1

pass=0; fail=0
result() {  # result <id> <description> <0=pass|1=fail> [detail]
    if [ "$3" -eq 0 ]; then
        printf '[PASS] %-11s %s\n' "$1" "$2"; pass=$((pass + 1))
    else
        printf '[FAIL] %-11s %s  -- %s\n' "$1" "$2" "${4:-}"; fail=$((fail + 1))
    fi
}
hex() { od -An -tx1 "$@" | tr -d ' \n'; }   # bytes as one hex string
size() { wc -c < "$1" | tr -d ' '; }

# ---------- test data ----------
printf 'abracadabra' > abra.txt
printf 'AAAAAAAAAAAAAAAAAAAA' > single.txt
: > empty.txt
seq 1 50000 > numbers.txt
head -c 100000 /dev/urandom > random.bin
awk 'BEGIN { for (i = 0; i < 4; i++) for (b = 1; b < 256; b++) printf "%c", b }' > bytes.bin

# ---------- REQ-1: accept a valid file path ----------
"$HZ" -c abra.txt -o abra.huff > out.log 2>&1; rc=$?
[ $rc -eq 0 ] && [ -s abra.huff ]
result TC-COMP-01 "Valid input path is accepted and compressed (REQ-1)" $? "exit=$rc"

# ---------- REQ-2/3/4: frequency table, Huffman codes, .huff layout (FORMAT.md worked example) ----------
freq_a=$(hex -j 784 -N 8 abra.huff)          # frequency of 'a' (0x61) at offset 8 + 8*97
freq_r=$(hex -j 920 -N 8 abra.huff)          # frequency of 'r' (0x72) at offset 8 + 8*114
[ "$freq_a" = "0500000000000000" ] && [ "$freq_r" = "0200000000000000" ]
result TC-COMP-02 "Frequency table in header matches input counts (a=5, r=2) (REQ-2)" $? "a=$freq_a r=$freq_r"

bits=$(hex -j 2056 -N 8 abra.huff); payload=$(hex -j 2064 abra.huff)
[ "$bits" = "1700000000000000" ] && [ "$payload" = "69cf68" ]
result TC-COMP-03 "Huffman encoding is deterministic: 23 bits, payload 69 CF 68 (REQ-3)" $? "bits=$bits payload=$payload"

"$HZ" -c numbers.txt > /dev/null 2>&1
osz=$(hex -N 8 abra.huff)
[ -f numbers.txt.huff ] && [ "$(size abra.huff)" -eq 2067 ] && [ "$osz" = "0b00000000000000" ]
result TC-COMP-04 "Output uses .huff extension and the FORMAT.md layout (2064-byte header) (REQ-4)" $? "size=$(size abra.huff)"

"$HZ" -c single.txt -o single.huff > /dev/null 2>&1 && "$HZ" -d single.huff -o single.out > /dev/null 2>&1 && cmp -s single.txt single.out
result TC-COMP-05 "Single-symbol file compresses and restores exactly (REQ-3)" $?

"$HZ" -c numbers.txt -o metrics.huff > metrics.log 2>&1
grep -q "Original size:" metrics.log && grep -q "Compressed size:" metrics.log && grep -q "Ratio:" metrics.log
result TC-COMP-06 "Original size, compressed size and ratio are displayed (SRS 2.2)" $?

"$HZ" -c empty.txt -o empty.huff > /dev/null 2>&1 && "$HZ" -d empty.huff -o empty.out > /dev/null 2>&1 \
    && [ "$(size empty.huff)" -eq 2064 ] && [ "$(size empty.out)" -eq 0 ]
result TC-COMP-07 "Empty file: header-only archive, restores to 0 bytes (edge case)" $?

# ---------- REQ-5/6/7: decompression ----------
"$HZ" -d abra.huff -o abra.out > dec.log 2>&1
grep -q "Decompressed size: 11 bytes" dec.log
result TC-DEC-01 "Header metadata is read: original size 11 bytes reported (REQ-5)" $?

cmp -s abra.txt abra.out
result TC-DEC-02 "Bitstream decodes back to 'abracadabra' (REQ-6)" $?

"$HZ" -d numbers.txt.huff -o numbers.out > /dev/null 2>&1; ok1=$?
"$HZ" -c random.bin -o random.huff > /dev/null 2>&1 && "$HZ" -d random.huff -o random.out > /dev/null 2>&1; ok2=$?
"$HZ" -c bytes.bin -o bytes.huff > /dev/null 2>&1 && "$HZ" -d bytes.huff -o bytes.out > /dev/null 2>&1; ok3=$?
[ $ok1 -eq 0 ] && [ $ok2 -eq 0 ] && [ $ok3 -eq 0 ] && cmp -s numbers.txt numbers.out && cmp -s random.bin random.out && cmp -s bytes.bin bytes.out
result TC-DEC-03 "Text, random binary and all-byte files restore byte-for-byte (REQ-7)" $?

d_default=$( "$HZ" -d abra.huff > /dev/null 2>&1; echo $? )
[ "$d_default" -eq 0 ] && cmp -s abra.txt abra.out
result TC-DEC-04 "Default output name <name>.out is used when -o is omitted (REQ-7)" $? "exit=$d_default"

cp abra.huff archive.bin
"$HZ" -d archive.bin > /dev/null 2>&1; rc=$?
[ $rc -eq 0 ] && cmp -s abra.txt archive.bin.out
result TC-DEC-05 "Archive without .huff extension restores to <name>.out (REQ-7)" $? "exit=$rc"

# ---------- error handling and safety ----------
"$HZ" -c does_not_exist.txt -o x.huff > err.log 2>&1; rc=$?
[ $rc -eq 2 ] && grep -q "not found" err.log && [ ! -e x.huff ]
result TC-ERR-01 "Nonexistent input: error message, exit code 2, no output (REQ-1, SAD 4.4)" $? "exit=$rc"

sz=$(size numbers.txt.huff); head -c $((sz - 40)) numbers.txt.huff > trunc.huff
printf 'KEEP' > keep.out
"$HZ" -d trunc.huff -o keep.out > /dev/null 2>&1; rc=$?
[ $rc -ne 0 ] && [ "$(cat keep.out)" = "KEEP" ] && [ ! -e keep.out.tmp ]
result TC-ERR-02 "Truncated archive rejected; existing output untouched (SRS 6.2)" $? "exit=$rc"

{ cat abra.huff; printf 'XYZ'; } > trailing.huff
"$HZ" -d trailing.huff -o t.out > /dev/null 2>&1; rc=$?
[ $rc -ne 0 ] && [ ! -e t.out ]
result TC-ERR-03 "Archive with trailing data is rejected (D-17)" $? "exit=$rc"

head -c 2064 single.huff > single_nopayload.huff
"$HZ" -d single_nopayload.huff -o s.out > /dev/null 2>&1; rc=$?
[ $rc -ne 0 ] && [ ! -e s.out ]
result TC-ERR-04 "Single-symbol archive without payload is rejected (D-17)" $? "exit=$rc"

cp abra.txt same.txt
"$HZ" -c same.txt -o same.txt > same.log 2>&1; rc=$?
[ $rc -eq 1 ] && cmp -s same.txt abra.txt && grep -q "must be different" same.log
result TC-ERR-05 "Output equal to input is refused; input file intact (D-14, SRS 6.2)" $? "exit=$rc"

"$HZ" > /dev/null 2>&1; r1=$?
"$HZ" -x abra.txt > /dev/null 2>&1; r2=$?
"$HZ" -c abra.txt -q out > /dev/null 2>&1; r3=$?
"$HZ" -h > help.log 2>&1; r4=$?
[ $r1 -eq 1 ] && [ $r2 -eq 1 ] && [ $r3 -eq 1 ] && [ $r4 -eq 0 ] && grep -q "Usage" help.log
result TC-ERR-06 "Wrong usage exits with code 1; -h shows usage (SRS 3.1)" $? "codes=$r1,$r2,$r3,$r4"

mkdir -p adir
"$HZ" -c adir -o d.huff > /dev/null 2>&1; rc=$?
[ $rc -eq 2 ] && [ ! -e d.huff ]
result TC-ERR-07 "A directory given as input is rejected (REQ-1)" $? "exit=$rc"

printf 'this is not a huff archive' > notarchive.huff
"$HZ" -d notarchive.huff -o n.out > /dev/null 2>&1; rc=$?
[ $rc -ne 0 ] && [ ! -e n.out ]
result TC-ERR-08 "A file that is not an archive is rejected (SRS 6.2)" $? "exit=$rc"

printf 'old' > existing.huff
"$HZ" -c abra.txt -o existing.huff > /dev/null 2>&1 && "$HZ" -d existing.huff -o e.out > /dev/null 2>&1 && cmp -s abra.txt e.out
result TC-ERR-09 "Existing output file is replaced safely via temp file + rename (D-14)" $?

# ---------- summary ----------
cd .. && rm -rf "$W"
echo "== System validation: $pass passed, $fail failed =="
[ "$fail" -eq 0 ]
