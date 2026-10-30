# Deviations from Submitted Documents

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Owner:** Pranavi Sistla (PES1UG24CS338)
**Last updated:** 30-Oct-2026 (final, development frozen at `v1.0`)

The Part-2 deliverables say: *"Deviations from the submitted document should be documented and explained during the final demo."* This file records every place where the implementation or process differs from what we submitted in Part 1:

| Baseline document | Version | Date |
|---|---|---|
| Software Requirements Specification (SRS) | 1.1 | 04-Oct-2026 |
| Software Architecture & Design Specification (SAD) | 1.0 | 06-Oct-2026 |
| Software Test Plan (STP) | 1.0 | 06-Oct-2026 |

**Status values:** ✅ *Accepted* (intentional, documents updated or no update needed) · 🔧 *Fix in progress* · ⏳ *Open* (still to be done) · 🔍 *To verify* (risk, being measured)

---

## Summary for the final demo

| ID | Area | Short description | Status |
|---|---|---|---|
| D-01 | Team | REQ-1/REQ-2 and REQ-4/REQ-7 ownership swapped between Pranavi and Inchara | ✅ Accepted (STP v1.1) |
| D-02 | Process | Board sprint names differ from the course's; decompression delivered one sprint early; STP schedule corrected in v1.1 | ✅ Accepted |
| D-03 | Repository | Repo moved to the `team14-file-compression` organisation; documents moved to `Documents/` | ✅ Accepted |
| D-04 | Technology | Implemented in C++17 only (SRS cites C++20; SAD says C/C++) | ✅ Accepted |
| D-05 | File format | `.huff` header has an extra 8-byte *bit count* field and a fixed-size frequency table | ✅ Accepted (SRS Appendix B update pending) |
| D-06 | Architecture | File writing is inside `codec.cpp`, not a separate File I/O module; interfaces and exit codes added | ✅ Accepted |
| D-07 | Behaviour | Very small and incompressible files get larger (2064-byte header) | ✅ Accepted |
| D-08 | Feature | Compression metrics were missing from the backlog; added late in Sprint 1 (PR #17) | ✅ Accepted |
| D-09 | Portability | Huffman tie-breaking differed between compilers, breaking cross-OS decompression | ✅ Fixed (PR #24) |
| D-10 | Memory NFR | Memory was about 1.6× the file size; fixed by streaming (PR #31), now within the SRS 6.1 limit | ✅ Fixed (PR #31) |
| D-11 | Performance NFR | 100 MiB: compress 4.35 s, decompress 0.57 s on a team laptop (target under 10 s) | ✅ Passed (TC-NF-01) |
| D-12 | Testing | Lightweight test runner plus GitHub Actions CI on 3 OSes instead of the tools named in the STP | ✅ Accepted |
| D-13 | Process | Three commits went directly to `main` before branch protection blocked bypassing | ✅ Accepted (fixed) |
| D-14 | Safety | Output could overwrite the input file; errors could crash; failed writes could leave partial files | ✅ Fixed (PR #23, PR #31) |
| D-15 | Team | REQ-7 (writing the restored file) was implemented inside Rachit's decompression PR #25 | ✅ Accepted |
| D-16 | Safety | Some corrupted payloads decode to wrong data without an error (no checksum in the format) | ✅ Accepted (known limitation) |
| D-17 | Safety | The streaming rewrite (PR #31) stopped rejecting trailing data and empty single-symbol payloads | ✅ Fixed (PR #33) |
| D-18 | Testing | Line endings: Git on Windows stores text files with CRLF, so "the same" README differs by platform | ✅ Accepted (explained) |
| D-19 | Team | #11 (lossless correctness check) was completed by Pranavi on freeze day instead of Rachit | ✅ Accepted |

---

## Details

### D-01: Ownership of REQ-1/REQ-2 and REQ-4/REQ-7 swapped
- **Submitted (SRS v1.1, Section 6 "Plan of Work"; STP v1.0, Section 9):**
  - Inchara: REQ-1, REQ-2, CLI and input validation.
  - Pranavi: REQ-4, REQ-7, memory management.
- **Actual:**
  - Pranavi: REQ-1, REQ-2 (CLI, input validation, frequencies) and portability (#9).
  - Inchara: REQ-4, REQ-7 (writing `.huff` and restored files) and memory (#10).
  - Pavan (REQ-3, performance) and Rachit (REQ-5, REQ-6, lossless correctness) are unchanged.
- **Reason:** Inchara handled the Phase 1 administration (Project board, SRS-to-backlog conversion, sprint mapping). The work was rebalanced when the backlog was assigned on 11-Oct-2026.
- **Where updated:**
  - GitHub issues #1, #2, #4, #7, #9, #10.
  - SRS Section 7 (ownership matrix).
- **STP:** Section 9 (Roles) corrected in **STP v1.1** (28-Oct-2026).

### D-02: Sprint naming and schedule
- **Course plan (Part-2 deliverables):**
  - Dry run: 12–16 Oct.
  - Sprint-1: 19–23 Oct.
  - Sprint-2: 26–30 Oct.
- **Our GitHub Project board:**
  - Iterations are named **Sprint 1** (dry run), **Sprint 2** (= course Sprint-1) and **Sprint 3** (= course Sprint-2).
  - Board Sprint 2 holds compression (#1–#4).
  - Board Sprint 3 holds decompression (#5–#7) and all NFRs (#8–#11).
- **What actually happened:** decompression (#5, #6, #7) was planned for board Sprint 3, but it was finished early and merged on 22-Oct (PR #25). The Sprint-1 video (`sprint1-demo.mp4`, PR #26, 60 s) therefore already shows the full round trip: compress, decompress, then `fc /b` reporting no differences, on Windows.
- **STP v1.0 Section 7** listed test execution on 06–07 Oct, before any code existed. Actual testing happens during the sprints. Corrected in **STP v1.1** (28-Oct-2026), which also updates the SRS references from 5.x to 6.x.

### D-03: Repository location and document folder
- **Repository moved:**
  - From `pranavi-9/file-compression-tool-basic-Zip-implementation-` to `team14-file-compression/file-compression-tool-basic-Zip-implementation-` on 14-Oct-2026, so the whole team manages it in one organisation.
  - All issues, pull requests and history were preserved, and the old URL redirects.
- **Documents moved:** all PDFs are now in `Documents/` (19-Oct-2026), with this file and `FORMAT.md`.

### D-04: Language standard
- **Submitted:**
  - SRS 1.4 references ISO/IEC 14882:2020 (C++20).
  - SRS 2.5 and SAD 3.6 say "C/C++".
- **Actual:** the code is **C++ only**, compiled with `-std=c++17` (see `Makefile` and `.github/workflows/build.yml`).
- **Reason:** no C++20 features are needed, and C++17 builds the same way with GCC (Linux), MinGW (Windows) and Apple Clang (macOS). It still satisfies the SRS constraint "developed strictly in C/C++".

### D-05: `.huff` file layout differs from SRS Appendix B
- **Submitted (SRS Appendix B):** three fields:
  - original file size (8 bytes)
  - frequency/tree metadata ("variable / 256 entries")
  - encoded bitstream
- **Actual** (fully specified in `Documents/FORMAT.md`): four fields, with a fixed 2064-byte header:
  1. original size: 8 bytes
  2. frequency table: always 256 × 8 = 2048 bytes
  3. **bit count**: 8 bytes (new)
  4. encoded bitstream
- **Reason:** the last byte of the bitstream is padded. Without the exact bit count, the decoder cannot tell padding bits from real data. A fixed-size table is simpler and safer to validate.
- **Also:**
  - REQ-4 allowed ".huff or .zip"; we use **`.huff`**.
  - Default output name is `<input>.huff` (e.g. `notes.txt` → `notes.txt.huff`).
  - Decompression defaults to `<name>.out`.
- **To do:** update SRS Appendix B in the next SRS revision.

### D-06: Module structure, interfaces and error codes vs SAD
- **Submitted (SAD 3.3/3.4):** five components: CLI, File I/O Module, Compression Engine, Huffman Tree Module, Decompression Engine. The File I/O Module both reads and writes files.
- **Actual mapping:**

| SAD component | Implementation |
|---|---|
| CLI | `src/main.cpp` |
| File I/O Module | `src/file_input.cpp` (reading). **Writing is done inside `src/codec.cpp`.** |
| Compression Engine | `src/frequency.cpp` and `compress_file()` in `src/codec.cpp` |
| Huffman Tree Module | `src/huffman.cpp`, `include/huffman.hpp` |
| Decompression Engine | `decompress_file()` in `src/codec.cpp` |

- **Added beyond the SAD:**
  - SAD 4.3 listed interfaces by name only. The actual function signatures are defined in `include/huffzip.h` and `include/huffman.hpp`.
  - SAD 4.4 did not define exit codes. We use **0** OK, **1** wrong usage, **2** file error, **3** other failure (corrupted input, etc.).
- **Reason:** keeping the `.huff` writing next to the format logic made REQ-4 simpler. The layered separation (CLI → engines → Huffman module) is unchanged.

### D-07: Small or incompressible files get larger
- **Submitted:** SRS 1.3 and 2.2 say the tool reduces file size.
- **Actual:**
  - Text files shrink well (a 1.3 MB text file became 0.56 MB).
  - Files under about 2–3 KB grow, because the header alone is 2064 bytes.
  - Random or already-compressed data (`.zip`, `.jpg`, `.mp4`) grows by 2064 bytes.
- **Reason:** this is expected for Huffman coding with a stored frequency table. It is not a defect. We will show it at the demo.

### D-08: Compression metrics added late
- **Submitted:** SRS 2.2, SAD 4.3 and STP TC-COMP-06: after compression, show the original size, compressed size and compression ratio.
- **What happened:** there was no backlog item for this feature, so the first compressor (PR #16, 20-Oct) printed only "Successfully compressed …". It was spotted in review on 21-Oct.
- **Fix:** Inchara added the metrics output to `compress_file()` in **PR #17** (22-Oct-2026). Example output:
  ```
  Original size:   588895 bytes
  Compressed size: 259012 bytes
  Ratio:           43.9827%
  ```
  The ratio is the compressed size as a percentage of the original, so lower is better.
- **Status:** ✅ Accepted. Retest TC-COMP-06 in Sprint 2.

### D-09: Huffman codes differ between compilers (cross-OS bug)
- **Requirement:** SRS 2.4 and 6.4 (portability on Windows, Linux and macOS) and SRS 6.4 (strictly lossless).
- **Found (21-Oct-2026):**
  - `build_tree()` in `src/huffman.cpp` orders nodes by frequency only.
  - When two nodes have equal frequency, the order depends on the C++ standard library.
  - **Tested:** for the same input, GCC's libstdc++ (Linux, Windows/MinGW) and Apple's libc++ (macOS) produced **different codes**. For `abracadabra`, `b` was `110` on one and `10` on the other.
  - The decompressor rebuilds the tree from the frequency table, so a file compressed on Windows or Linux would decompress **incorrectly** on a Mac.
- **Fix:**
  - Add a tie-break rule: on equal frequency, the node with the smaller byte value comes first.
  - An internal node uses the smallest byte value in its subtree.
  - Verified to give identical codes on both libraries, and all existing tests pass.
  - The rule is written into `FORMAT.md` Section 5.
- **Owner:** Pavan (REQ-3). **Fixed in PR #24** (22-Oct-2026), which adds a `min_byte` field to each node and orders ties by it.
- **Verified (24-Oct):** files compressed with the GCC/libstdc++ build decompress byte-identically with the Clang/libc++ build, and the other way round. That is 21 of 21 round trips over text, binary, empty, single-symbol and all-256-byte files. The `FORMAT.md` worked example still matches byte for byte.
- **Status:** ✅ Fixed. Confirmed on real machines on 29-Oct: Mac (Apple clang 21, arm64) ↔ Windows (g++ 16.2.0). Both directions are byte-identical (`Documents/PORTABILITY.md`, TC-NF-03 PASS).
- **Retest:** TC-NF-03. Compress on one OS, decompress on another, and compare with `cmp` or `fc /b`.

### D-10: Memory use (SRS 6.1)
- **Submitted:** SRS 6.1: "Memory consumption should not exceed the size of the file being processed plus tree overhead."
- **Before:**
  - The whole input and the whole compressed output were held in memory.
  - A 103 MB file used 165 MB to compress and 154 MB to decompress (about 1.6×).
- **Fix (Inchara, #10, PR #31, 28-Oct-2026):**
  - The encoded bits are written straight to the output file instead of being collected in memory.
  - Decompression reads the archive and writes the restored file as a stream.
- **Measured after the fix (28-Oct, 103 MB text file):**

| Operation | Peak memory | Compared with file size |
|---|---|---|
| Compress | **101 MB** | about 1.0× (the input is still loaded once to count frequencies) |
| Decompress | **10 MB** | about 0.1× |

- **Status:** ✅ Fixed. Both are within "file size + tree overhead".

### D-11: 100 MB performance (SRS 6.1)
- **Submitted:** SRS 6.1: compress and decompress 100 MB in under 10 s.
- **Official result (Pavan, #8, `Documents/TC-NF-01_Performance_Results.md`):**
  - 100 MiB file, Windows, Intel Core i5-12450H, 15.7 GiB RAM, g++ 16.2.0.
  - Averages over 3 runs: **compress 4.35 s**, **decompress 0.57 s**.
  - All three outputs were byte-identical (`fc /b`). **TC-NF-01: PASS.**
- **Note:** that benchmark used the build from 26-Oct, before the streaming change in PR #31. A recheck on 28-Oct (Linux test machine, 103 MB) gave compress 1.8 s and decompress 4.1 s, still well under 10 s.
- **Status:** ✅ Passed.

### D-12: Test tools and CI
- **Submitted:** STP Sections 5 and 6 name generic "byte comparison, time and memory measurement tools". SAD 5.3 lists no CI.
- **Actual:**
  - Unit tests use a small built-in runner, with no external libraries (SAD 3.6: "External Libraries: Not required").
    - `tests/test_main.cpp` calls each member's tests.
    - Implemented so far: TC-COMP-01, TC-ERR-01, TC-COMP-02 (`tests/test_input.cpp`), plus Huffman tree, single-symbol and encoding checks for TC-COMP-03 and TC-COMP-05 (`tests/test_huffman.cpp`).
  - **GitHub Actions** (`.github/workflows/build.yml`) builds and runs all tests on **Ubuntu, Windows and macOS** for every push and pull request. This also gives evidence for TC-NF-03 (portability).
  - `main` is protected: every change needs a pull request, passing checks and 1 approval.
- **Reason:** the Part-2 deliverables require GitHub Actions. Running on 3 OSes is what exposed D-09.

### D-13: Direct commits to `main`
- **Expected (Part-2 deliverables):** the assigned member raises a PR, and the repo owner reviews and merges.
- **Actual:**
  - Branch protection was added on 19-Oct-2026.
  - Three documentation commits made on 19–20 Oct went directly to `main`: removing the old PDFs, adding the `Documents/` folder and updating the README. Organisation owners could still bypass the rule.
  - On 21-Oct-2026 the rule was changed to **"Do not allow bypassing"**. Since then, all changes go through pull requests.
- **Status:** ✅ Fixed.

### D-14: Safety gaps in file handling
- **Submitted:** SRS 6.2: the program must "terminate gracefully with a helpful error message rather than crashing or corrupting existing user data". STP 5.1 also requires that a failed write does not damage an existing file.
- **Found (21-Oct-2026 code review):**
  - `huffzip -c notes.txt -o notes.txt` replaced the original file with its compressed version, so the **original data was lost**.
  - An out-of-memory or overflow error inside the Huffman code was not caught, so the program would crash instead of printing an error.
- **Fix (Pranavi, CLI owner, REQ-1):**
  - `src/main.cpp` now refuses when the output file is the same as the input file (exit code 1).
  - It catches unexpected errors and exits with a message (exit code 3).
  - The CI smoke test checks both cases on all 3 OSes.
- **Completed in PR #31 (Inchara, 28-Oct):** compression and decompression now write to `<output>.tmp` and rename it only when everything succeeded. A failed or corrupted run never leaves a partial file and never replaces an existing one (verified 28-Oct). An existing output file is still replaced without asking when the run succeeds, which is normal behaviour for command-line tools.
- **Status:** ✅ Fixed in PR #23 and PR #31. Re-verified 28-Oct: decompressing onto the input file is refused, and a corrupted archive leaves the existing output untouched with no `.tmp` file left behind.

### D-15: REQ-7 implemented inside the decompression PR
- **Submitted:**
  - SRS Section 7: REQ-7 (write the decompressed data to a new file) is Inchara's.
  - Issue #7 is assigned to Inchara.
- **Actual:**
  - Rachit's PR #25 (REQ-5, REQ-6) also writes the restored file in `decompress_file()`, so REQ-7's code arrived together with decoding.
  - It checks that the decoded size equals the original size.
  - It writes in binary mode.
  - It only opens the output after decoding succeeds, so a corrupted archive never damages an existing file (verified 24-Oct).
- **Reason:** decoding and writing live in the same function, and splitting them across two PRs would have caused merge conflicts.
- **Action:**
  - Inchara reviews the REQ-7 part and adds the remaining safety item from D-14 (write to a temporary file, then rename), under issue #7.
  - Update issue #7 to reflect this.
- **Status:** ✅ Accepted.

### D-16: Not every corruption is detected
- **Submitted:**
  - SRS 6.2: "If … a file is corrupted, the program must terminate gracefully with a helpful error message."
  - STP TC-ERR-02: "Corruption is detected/handled safely."
- **Actual (tested 24-Oct-2026):**
  - Detected and rejected with a clear error, with no output written:
    - truncated archives
    - wrong frequency table
    - a file that is not an archive
    - a header shorter than 2064 bytes
  - **But** of 200 random single-bit flips inside the payload, **140 decoded without any error into wrong data**. A flipped bit often turns one valid Huffman code into another, and the format has no checksum to notice.
- **Options for the team:**
  1. **Add a CRC32 checksum** of the original data to the header (4 more bytes). The decoder compares it after decoding. That changes `FORMAT.md`, `compress_file()` (Inchara) and `decompress_file()` (Rachit), plus one test. It is about 40 lines in total.
  2. **Accept and explain:** "structural corruption is detected; silent payload corruption is a known limitation of plain Huffman coding without a checksum." In that case, test TC-ERR-02 with truncation or header damage.
- **Decision (29-Oct-2026):** option 2. With the 30-Oct freeze, the team chose not to change the file format this late.
  - **What is detected:** structural corruption (truncation, damaged header, wrong frequency table, extra trailing data, missing single-symbol payload) is rejected with a clear error, and no output file is written (D-14, D-17).
  - **What is not:** a bit flip inside the payload can still decode into wrong data. Plain Huffman coding has no integrity check.
  - **Future work:** a CRC32 of the original data in the header (about 4 bytes and 40 lines of code).
- **Status:** ✅ Accepted (known limitation, to be explained at the demo).

### D-17: Two checks lost in the streaming rewrite
- **Found (28-Oct-2026 review of PR #31):** when decompression switched to streaming, two checks from PR #28 disappeared:
  1. An archive with **extra bytes after the bitstream** decompressed "successfully" instead of being rejected.
  2. A **single-symbol** archive (e.g. `AAAAAAAAAA`) with its payload cut off decompressed "successfully", because the payload was never read.
- **Fix (Rachit, REQ-5/REQ-6, PR #33, 29-Oct-2026):** `decompress_file()` checks for trailing data and reads and validates the single-symbol payload. Two new unit tests cover both cases. All tests pass, and the cross-compiler round trips are still identical.
- **Status:** ✅ Fixed.

### D-18: Line endings differ between Windows and Mac/Linux checkouts
- **Context:** TC-NF-03 (portability) exchanges archives between Pavan's Windows laptop and Pranavi's Mac.
- **Observation:** Pavan's `win.huff` (made from his `README.md`) decompresses to 2128 bytes, while `README.md` on the Mac is 2070 bytes. The difference is exactly the 58 carriage returns (`\r`) that Git for Windows adds to each line of text files when it checks them out (`core.autocrlf`).
- **Conclusion:** this is **not** a tool defect. Removing the `\r` characters gives a file identical to the Mac copy, and the tool restores every byte it was given. For the portability evidence, the restored file is compared with the **sender's own** copy of the input (see `PORTABILITY.md`). Pavan's `README_win.md` (2128 bytes, 58 CRLF line endings) matched the file restored on the Mac byte for byte.
- **Status:** ✅ Accepted (explained).

### D-19: #11 completed by the repo owner on freeze day
- **Submitted:**
  - SRS Section 7: Rachit owns lossless correctness (100% byte-for-byte match).
  - Issue #11 is assigned to Rachit.
- **What happened:** Rachit's main contributions were decompression (PR #25) and its validation fixes (PR #33), which already include round-trip unit tests (`tests/test_codec.cpp`). The separate #11 check script was not started by freeze day (30-Oct-2026), so Pranavi (repo owner) completed it to avoid blocking the release.
- **What was delivered:** `tests/lossless_check.sh`, which also runs automatically in GitHub Actions on Linux, Windows and macOS:
  - **12 round trips**, each compared byte for byte with `cmp`: empty, 1 byte, 2 symbols, single symbol, `abracadabra`, Windows CRLF text, 1.3 MB numbers, 230 KB English text, 500 KB random data, all 256 byte values, README and source code.
  - **5 corruption cases**: truncated, short header, trailing data, not an archive, single-symbol without payload. Each must be rejected and must leave no output file.
  - **Result (30-Oct, `main` @ `7a826e6`): 17 passed, 0 failed.** This covers STP TC-DEC-03 and TC-ERR-02.
- **Status:** ✅ Accepted.

---

## How to add a deviation

Whenever a PR changes something that the SRS, SAD or STP describes differently, add a row to the summary table and a section below it, using this template:

```
### D-xx: <short title>
- **Submitted:** <document, section, what it says>
- **Actual:** <what we built or did>
- **Reason:** <why>
- **Action / Owner:** <what is being done, by whom, PR number>
- **Status:** ✅ Accepted / 🔧 Fix in progress / ⏳ Open / 🔍 To verify
```

**Review points:** end of Sprint 1 (23-Oct), end of Sprint 2 (30-Oct), and before the final demo.
