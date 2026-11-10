# System Validation Report

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Date of execution:** 05-Nov-2026
**Code version:** `main` @ `cae72ce` + Sprint-3 tests
**Related:** Test Case Document v2.0 (Section 3), STP v1.1 (Sections 3, 5, 5.1), SRS v1.1

---

## 1. Purpose and scope

This report validates the complete `huffzip` program against the SRS requirements, using the built command-line tool (black-box), as defined in STP Section 5 "System testing". It covers:
- functional requirements **REQ-1 to REQ-7** and SRS 2.2 (metrics)
- error handling and safety (**SRS 6.2**, SAD 3.9 / 4.4, STP 5.1)
- non-functional requirements: performance and memory (**SRS 6.1**), portability and lossless correctness (**SRS 2.4 / 6.4**)

## 2. Summary

| Suite | Script / evidence | Test cases | Passed | Failed | Known limitation |
|---|---|---|---|---|---|
| Functional and error handling | `tests/system_validation.sh` | 21 | 21 | 0 | – |
| Lossless and corruption (#11) | `tests/lossless_check.sh` | 17 | 17 | 0 | – |
| Non-functional | TC-NF-01, 02, 03 | 3 | 3 | 0 | – |
| Security (payload bit flips) | TC-SEC-01 | 1 | – | – | 1 (D-16) |
| **Total** | | **42** | **41** | **0** | **1** |

**Result: the system meets REQ-1 to REQ-7, SRS 2.2, 2.4, 6.1, 6.2 and 6.4.** The one known limitation (TC-SEC-01) is documented in DEVIATIONS D-16.

## 3. Test environment

| Item | Value |
|---|---|
| Linux test machine | Ubuntu 24.04.5 LTS, x86-64, GCC 13.3.0 |
| Team laptops | macOS, Apple Silicon (arm64), Apple clang 21.0.0 (Pranavi); Windows 11, Intel Core i5-12450H, 15.7 GiB RAM, MSYS2 g++ 16.2.0 (Pavan) |
| CI | GitHub Actions: Ubuntu, Windows, macOS. Both scripts run on every push and pull request |
| Build | `make` → `huffzip` (`-std=c++17 -O2`) |
| Commands | `bash tests/system_validation.sh` · `bash tests/lossless_check.sh` |

---

## 4. Functional and error-handling results (`system_validation.sh`)

| ID | Requirement | Validated behaviour | Result |
|---|---|---|---|
| TC-COMP-01 | REQ-1 | A valid input path is accepted and compressed | PASS |
| TC-COMP-02 | REQ-2 | Frequency table in the header matches the input (a=5, r=2) | PASS |
| TC-COMP-03 | REQ-3 | Deterministic encoding: 23 bits, payload `69 CF 68` (FORMAT.md §7) | PASS |
| TC-COMP-04 | REQ-4 | `.huff` extension and FORMAT.md layout (2064-byte header) | PASS |
| TC-COMP-05 | REQ-3 | Single-symbol file compresses and restores exactly | PASS |
| TC-COMP-06 | SRS 2.2 | Original size, compressed size and ratio displayed | PASS |
| TC-COMP-07 | SAD 4.6 | Empty file → header-only archive → 0-byte restore | PASS |
| TC-DEC-01 | REQ-5 | Header read; original size (11 bytes) reported | PASS |
| TC-DEC-02 | REQ-6 | Bitstream decodes back to `abracadabra` | PASS |
| TC-DEC-03 | REQ-7 | Text, random binary and all-byte files restore byte for byte | PASS |
| TC-DEC-04 | REQ-7 | Default output name `<name>.out` | PASS |
| TC-DEC-05 | REQ-7 | Archive without `.huff` extension → `<name>.out` | PASS |
| TC-ERR-01 | REQ-1, SAD 4.4 | Missing input: message, exit 2, no output | PASS |
| TC-ERR-02 | SRS 6.2 | Truncated archive rejected; existing output untouched; no `.tmp` | PASS |
| TC-ERR-03 | SRS 6.2, D-17 | Trailing data rejected | PASS |
| TC-ERR-04 | SRS 6.2, D-17 | Single-symbol archive without payload rejected | PASS |
| TC-ERR-05 | SRS 6.2, D-14 | Output equal to input refused; input intact | PASS |
| TC-ERR-06 | SRS 3.1 | Wrong usage → exit 1; `-h` shows usage | PASS |
| TC-ERR-07 | REQ-1 | Folder given as input rejected | PASS |
| TC-ERR-08 | SRS 6.2 | Non-archive file rejected | PASS |
| TC-ERR-09 | D-14 | Existing output replaced safely (temp file + rename) | PASS |

**Effectiveness check (negative control):** the same script was run against an older build from 28-Oct (`2895a8a`), before the D-17 fix. It correctly reported **2 failures (TC-ERR-03, TC-ERR-04)**. This shows the tests detect real defects and do not just pass.

---

## 5. Lossless correctness and corruption (`lossless_check.sh`, issue #11, TC-NF-04)

| # | Input | Original → compressed | Restored identical (`cmp`) |
|---|---|---|---|
| 1 | `abracadabra` | 11 B → 2067 B | ✅ |
| 2 | All 256 byte values × 20 | 5120 B → 7184 B | ✅ |
| 3 | Empty file | 0 B → 2064 B | ✅ |
| 4 | English text | 226,893 B → 137,357 B (39% smaller) | ✅ |
| 5 | Numbers 1–200000 | 1,288,895 B → 559,012 B (57% smaller) | ✅ |
| 6 | One byte | 1 B → 2065 B | ✅ |
| 7 | Random data | 500,000 B → 502,064 B (incompressible) | ✅ |
| 8 | README.md | 2070 B → 3298 B | ✅ |
| 9 | Single symbol × 40 | 40 B → 2069 B | ✅ |
| 10 | Source code (`codec.cpp`) | 12,320 B → 9,132 B (26% smaller) | ✅ |
| 11 | Two symbols | 2 B → 2065 B | ✅ |
| 12 | Windows CRLF text | 32 B → 2078 B | ✅ |

| # | Damaged archive | Expected | Result |
|---|---|---|---|
| 13 | Not an archive | Rejected, no output | ✅ |
| 14 | Header only 100 bytes | Rejected, no output | ✅ |
| 15 | Single-symbol archive without payload | Rejected, no output | ✅ |
| 16 | Trailing garbage | Rejected, no output | ✅ |
| 17 | Truncated by 50 bytes | Rejected, no output | ✅ |

Small files grow because of the fixed 2064-byte header, and random data does not shrink. This is expected (DEVIATIONS D-07).

---

## 6. Non-functional validation

### TC-NF-01: Performance (SRS 6.1: under 10 s for 100 MB): **PASS**
| Machine | File | Compress | Decompress | Restored |
|---|---|---|---|---|
| Windows 11, i5-12450H (Pavan, 3-run average; `TC-NF-01_Performance_Results.md`) | 100 MiB | **4.35 s** | **0.57 s** | identical (`fc /b`) |
| Linux test machine, 05-Nov (2 runs) | 103 MB text | 1.93 s / 2.24 s | 3.61 s / 3.51 s | identical |

### TC-NF-02: Memory (SRS 6.1: at most file size + tree overhead): **PASS**
| Operation (103 MB text file, 05-Nov) | Peak memory (RSS) | Compared with file size |
|---|---|---|
| Compress | 101 MB | 0.98× |
| Decompress | 11 MB | 0.11× |

Before PR #31 this was 165 MB / 154 MB (about 1.6×). The streaming I/O fixed it (D-10).

### TC-NF-03: Portability (SRS 2.4 / 6.4): **PASS**
- **CI:** build, unit tests and smoke tests are green on Ubuntu, Windows and macOS (run #46: `actions/runs/40862046404`).
- **Real machines:** an archive made on the Mac decompressed on Windows with `fc /b` "no differences encountered", and the reverse direction gave an identical `cmp`. Replacing an existing output also works on Windows. Details: `PORTABILITY.md`.
- **Cross-compiler:** 22/22 round trips between GCC/libstdc++ and Clang/libc++ are byte-identical.

---

## 7. Known limitation: TC-SEC-01 (DEVIATIONS D-16)

| Test | Result (05-Nov) |
|---|---|
| 200 single-bit flips at random positions in the payload of a valid archive | **60 rejected** with an error; **140 decoded without an error into wrong data** |

**Why:** a flipped bit often turns one valid Huffman code into another, and the `.huff` format has no checksum. **Structural** corruption (truncation, header damage, trailing data, non-archives) is always rejected: TC-ERR-02/03/04/08 and lossless checks 13–17.
**Decision:** accepted as a known limitation for v1.0. **Future work:** add a CRC32 of the original data to the header.

---

## 8. Conclusion

All 41 executed system, lossless and non-functional test cases pass on the current code, on the Linux test machine, in CI on three operating systems, and on the team's Mac and Windows laptops. The STP v1.1 exit criteria are met:

| STP exit criterion | Status |
|---|---|
| Planned tests executed | ✅ 100% (42 system/NF/security cases and 36 unit tests) |
| All requirements have test coverage | ✅ (RTM in Test Case Document v2.0, Section 5) |
| No critical defect open | ✅ (D-16 documented as a known limitation) |
| Performance results recorded | ✅ (TC-NF-01, TC-NF-02) |

---

## Appendix: console output (05-Nov-2026)

```
$ bash tests/system_validation.sh
[PASS] TC-COMP-01  Valid input path is accepted and compressed (REQ-1)
[PASS] TC-COMP-02  Frequency table in header matches input counts (a=5, r=2) (REQ-2)
[PASS] TC-COMP-03  Huffman encoding is deterministic: 23 bits, payload 69 CF 68 (REQ-3)
[PASS] TC-COMP-04  Output uses .huff extension and the FORMAT.md layout (2064-byte header) (REQ-4)
[PASS] TC-COMP-05  Single-symbol file compresses and restores exactly (REQ-3)
[PASS] TC-COMP-06  Original size, compressed size and ratio are displayed (SRS 2.2)
[PASS] TC-COMP-07  Empty file: header-only archive, restores to 0 bytes (edge case)
[PASS] TC-DEC-01   Header metadata is read: original size 11 bytes reported (REQ-5)
[PASS] TC-DEC-02   Bitstream decodes back to 'abracadabra' (REQ-6)
[PASS] TC-DEC-03   Text, random binary and all-byte files restore byte-for-byte (REQ-7)
[PASS] TC-DEC-04   Default output name <name>.out is used when -o is omitted (REQ-7)
[PASS] TC-DEC-05   Archive without .huff extension restores to <name>.out (REQ-7)
[PASS] TC-ERR-01   Nonexistent input: error message, exit code 2, no output (REQ-1, SAD 4.4)
[PASS] TC-ERR-02   Truncated archive rejected; existing output untouched (SRS 6.2)
[PASS] TC-ERR-03   Archive with trailing data is rejected (D-17)
[PASS] TC-ERR-04   Single-symbol archive without payload is rejected (D-17)
[PASS] TC-ERR-05   Output equal to input is refused; input file intact (D-14, SRS 6.2)
[PASS] TC-ERR-06   Wrong usage exits with code 1; -h shows usage (SRS 3.1)
[PASS] TC-ERR-07   A directory given as input is rejected (REQ-1)
[PASS] TC-ERR-08   A file that is not an archive is rejected (SRS 6.2)
[PASS] TC-ERR-09   Existing output file is replaced safely via temp file + rename (D-14)
== System validation: 21 passed, 0 failed ==

$ bash tests/lossless_check.sh
== Result: 17 passed, 0 failed ==
```
