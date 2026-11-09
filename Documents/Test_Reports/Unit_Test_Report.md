# Unit Test Report

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Date of execution:** 05-Nov-2026
**Code version:** `main` @ `cae72ce` + Sprint-3 tests (`tests/test_robustness.cpp`)
**Related:** Test Case Document v2.0 (Section 4), STP v1.1 Section 5 (Unit testing)

---

## 1. Summary

| Test group | File | Owner | Tests | Passed | Failed |
|---|---|---|---|---|---|
| Huffman tree, codes, encoding | `tests/test_huffman.cpp` | Pavan | 7 | 7 | 0 |
| Input validation and frequencies | `tests/test_input.cpp` | Pranavi | 4 | 4 | 0 |
| Codec round trips and rejection | `tests/test_codec.cpp` | Rachit | 7 | 7 | 0 |
| Robustness and error paths | `tests/test_robustness.cpp` | Sprint 3 | 18 | 18 | 0 |
| **Total** | | | **36** | **36** | **0** |

**Result: PASS (36/36).** The runner exits with code 0.

---

## 2. Test environment

| Item | Value |
|---|---|
| Test runner | Built-in runner, no external libraries (`tests/test_main.cpp`, SAD 3.6) |
| Command | `make test` (builds `run_tests` from `src/*.cpp` except `main.cpp`, plus `tests/*.cpp`) |
| Machine 1 | Ubuntu 24.04.5 LTS, x86-64, GCC 13.3.0 (`-std=c++17 -Wall -Wextra -O2`) |
| Machine 2 | Same machine, Clang 18.1.3 with libc++ (the standard library used on macOS) |
| Memory/UB check | GCC with `-fsanitize=address,undefined` |
| CI | GitHub Actions on `ubuntu-latest`, `windows-latest`, `macos-latest` (`.github/workflows/build.yml`), every push and pull request |

| Build | Warnings | Result |
|---|---|---|
| GCC 13.3.0, `-Wall -Wextra -Wpedantic` | 0 | 36/36 PASS |
| Clang 18.1.3 + libc++ | 0 | 36/36 PASS |
| GCC + AddressSanitizer + UBSan | 0 sanitizer errors | 36/36 PASS |

---

## 3. Detailed results

### 3.1 Huffman module (`src/huffman.cpp`): REQ-3

| ID | Test function | What is checked | Result |
|---|---|---|---|
| UT-HUF-01 | `test_empty_frequencies` | All frequencies zero → `build_tree` returns no tree | PASS |
| UT-HUF-02 | `test_single_symbol` | One symbol → the tree is a single leaf, code `"0"` | PASS |
| UT-HUF-03 | `test_basic_huffman_codes` | A=5, B=2, C=1 → every symbol gets a code | PASS |
| UT-HUF-04 | `test_deterministic_tie_breaking` | Four equal frequencies → exactly A=`00`, B=`01`, C=`10`, D=`11` on every compiler (D-09) | PASS |
| UT-HUF-05 | `test_binary_symbols` | Byte values 0 and 255 encode; packed size = ⌈bits/8⌉ | PASS |
| UT-HUF-06 | `test_encoded_size_consistency` | Packed byte count matches the bit count | PASS |
| UT-HUF-07 | `test_missing_code_throws` | Encoding a byte that has no code throws `invalid_argument` | PASS |

### 3.2 Input and frequency module (`src/file_input.cpp`, `src/frequency.cpp`): REQ-1, REQ-2

| ID | Test function | Test case | What is checked | Result |
|---|---|---|---|---|
| UT-IN-01 | `tc_comp_01_valid_path` | TC-COMP-01 | An existing file is accepted by `validate_input_file` | PASS |
| UT-IN-02 | `tc_err_01_invalid_path` | TC-ERR-01 | A missing file and an empty path are rejected with a "not found" message | PASS |
| UT-IN-03 | `tc_comp_02_frequencies` | TC-COMP-02 | `aaabbc\n` → a=3, b=2, c=1, `\n`=1, z=0; file read in binary mode (7 bytes) | PASS |
| UT-IN-04 | `tc_comp_02_binary_and_empty` | TC-COMP-02 | All 256 byte values counted once; empty input gives all zeros | PASS |

### 3.3 Codec (`src/codec.cpp`): REQ-4 to REQ-7

| ID | Test | What is checked | Result |
|---|---|---|---|
| UT-COD-01 | Round trip: text | `compress_file` → `decompress_file` → bytes identical | PASS |
| UT-COD-02 | Round trip: empty | 0-byte file restores to 0 bytes | PASS |
| UT-COD-03 | Round trip: binary | 4 × all 256 byte values restore exactly | PASS |
| UT-COD-04 | Round trip: single byte | 1000 × `A` restores exactly | PASS |
| UT-COD-05 | Rejects corrupted metadata | Original-size field zeroed → rejected, no output file | PASS |
| UT-COD-06 | Rejects trailing data | One extra byte after the payload → rejected (D-17) | PASS |
| UT-COD-07 | Rejects truncated single-symbol archive | Header only, payload removed → rejected (D-17) | PASS |

### 3.4 Robustness and error paths (Sprint 3): SRS 6.2, SAD 4.4, STP 5.1

Each test damages an archive or blocks an output and checks for **the right exit code and no output file or `.tmp` file left behind**.

| ID | What is checked | Expected | Result |
|---|---|---|---|
| UT-ROB-01 | Archive file does not exist | `EXIT_FILE_ERROR` (2) | PASS |
| UT-ROB-02 | Header cut inside the original-size field (4 bytes) | Rejected (3) | PASS |
| UT-ROB-03 | Header cut inside the frequency table (500 bytes) | Rejected (3) | PASS |
| UT-ROB-04 | Header cut inside the bit-count field (2060 bytes) | Rejected (3) | PASS |
| UT-ROB-05 | Frequencies add up past 2⁶⁴ (overflow) | Rejected (3) | PASS |
| UT-ROB-06 | Empty archive claiming a non-zero bit count | Rejected (3) | PASS |
| UT-ROB-07 | Non-empty archive claiming zero bits | Rejected (3) | PASS |
| UT-ROB-08 | Single-symbol archive with the wrong bit count | Rejected (3) | PASS |
| UT-ROB-09 | Single-symbol payload containing a `1` bit | Rejected (3) | PASS |
| UT-ROB-10 | Bit count one short (stream ends inside a code) | Rejected (3) | PASS |
| UT-ROB-11 | Extra encoded symbols beyond the original size | Rejected (3) | PASS |
| UT-ROB-12 | Payload missing its last byte | `EXIT_FILE_ERROR` (2) | PASS |
| UT-ROB-13 | Bit count larger than the payload | Rejected (non-zero) | PASS |
| UT-ROB-14 | Compress a file that cannot be read | `EXIT_FILE_ERROR` (2), no output | PASS |
| UT-ROB-15 | Compress into a folder that does not exist | `EXIT_FILE_ERROR` (2) | PASS |
| UT-ROB-16 | Decompress into a folder that does not exist | `EXIT_FILE_ERROR` (2) | PASS |
| UT-ROB-17 | `build_tree` with frequencies that overflow when added | Throws `overflow_error` | PASS |
| UT-ROB-18 | `encode` with a code containing a character other than 0/1 | Throws `invalid_argument` | PASS |

---

## 4. Defects found by unit and system testing during the project

| Defect | Found by | Fixed in | Regression test |
|---|---|---|---|
| Huffman codes differed between GCC and Clang/libc++ (cross-OS decoding failed) | Cross-compiler round trip, 21-Oct | PR #24 | UT-HUF-04, TC-NF-03 |
| Output could overwrite the input file | Code review / CLI testing, 21-Oct | PR #23 | TC-ERR-05 |
| Trailing data and empty single-symbol payloads accepted after the streaming rewrite | Corruption tests, 28-Oct | PR #33 | UT-COD-06, UT-COD-07, TC-ERR-03, TC-ERR-04 |

---

## 5. How to reproduce

```bash
make clean
make test
```
Expected last line: `All tests passed`.

---

## Appendix: console output (05-Nov-2026, GCC 13.3.0)

Compression/decompression progress messages are omitted; all `[PASS]` lines are shown.

```
Running project tests...
[PASS] Huffman tests                                   (UT-HUF-01..07)
[PASS] Input and frequency tests (TC-COMP-01, TC-ERR-01, TC-COMP-02)   (UT-IN-01..04)
[PASS] Codec round trip: text
[PASS] Codec round trip: empty
[PASS] Codec round trip: binary
[PASS] Codec round trip: single_byte
[PASS] Codec rejects corrupted metadata
[PASS] Codec rejects trailing data
[PASS] Codec rejects truncated single-symbol archive
[PASS] UT-ROB-01 missing archive file -> EXIT_FILE_ERROR
[PASS] UT-ROB-02 header cut inside original-size field is rejected
[PASS] UT-ROB-03 header cut inside frequency table is rejected
[PASS] UT-ROB-04 header cut inside bit-count field is rejected
[PASS] UT-ROB-05 frequency total overflow is rejected
[PASS] UT-ROB-06 empty archive with non-zero bit count is rejected
[PASS] UT-ROB-07 non-empty archive with zero bit count is rejected
[PASS] UT-ROB-08 single-symbol archive with wrong bit count is rejected
[PASS] UT-ROB-09 single-symbol payload containing a 1-bit is rejected
[PASS] UT-ROB-10 bitstream ending in the middle of a code is rejected
[PASS] UT-ROB-11 extra encoded symbols beyond the original size are rejected
[PASS] UT-ROB-12 payload missing its last byte is rejected (truncated bitstream)
[PASS] UT-ROB-13 bit count larger than the payload is rejected
[PASS] UT-ROB-14 compress with unreadable input -> EXIT_FILE_ERROR, no output
[PASS] UT-ROB-15 compress into a missing folder -> EXIT_FILE_ERROR
[PASS] UT-ROB-16 decompress into a missing folder -> EXIT_FILE_ERROR
[PASS] UT-ROB-17 build_tree detects frequency overflow
[PASS] UT-ROB-18 encode rejects a code containing characters other than 0/1
All tests passed
```
