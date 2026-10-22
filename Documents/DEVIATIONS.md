# Deviations from Submitted Documents

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Owner:** Pranavi Sistla (PES1UG24CS338)
**Last updated:** 22-Oct-2026 (during Sprint 1)

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
| D-01 | Team | REQ-1/REQ-2 and REQ-4/REQ-7 ownership swapped between Pranavi and Inchara | ✅ Accepted (STP update pending) |
| D-02 | Process | Board sprint names and planned timing differ from the course sprint names; STP schedule outdated | ✅ Accepted |
| D-03 | Repository | Repo moved to the `team14-file-compression` organisation; documents moved to `Documents/` | ✅ Accepted |
| D-04 | Technology | Implemented in C++17 only (SRS cites C++20; SAD says C/C++) | ✅ Accepted |
| D-05 | File format | `.huff` header has an extra 8-byte *bit count* field and a fixed-size frequency table | ✅ Accepted (SRS Appendix B update pending) |
| D-06 | Architecture | File writing is inside `codec.cpp`, not a separate File I/O module; interfaces and exit codes added | ✅ Accepted |
| D-07 | Behaviour | Very small and incompressible files get larger (2064-byte header) | ✅ Accepted |
| D-08 | Feature | Compression metrics were missing from the backlog; added late in Sprint 1 (PR #17) | ✅ Accepted |
| D-09 | Portability | Huffman tie-breaking differs between compilers, breaking cross-OS decompression | 🔧 Fix in progress |
| D-10 | Memory NFR | Whole file plus compressed output held in memory, which may exceed the SRS 6.1 limit | 🔍 To verify |
| D-11 | Performance NFR | 100 MB in under 10 s not yet measured | 🔍 To verify |
| D-12 | Testing | Lightweight test runner plus GitHub Actions CI on 3 OSes instead of the tools named in the STP | ✅ Accepted |
| D-13 | Process | Three commits went directly to `main` before branch protection blocked bypassing | ✅ Accepted (fixed) |
| D-14 | Safety | Output could overwrite the input file; unexpected errors could crash instead of exiting cleanly | 🔧 Fix in progress |

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
- **Still to do:** STP Section 9 (Roles) still shows the old split and will be corrected in STP v1.1.

### D-02: Sprint naming and schedule
- **Course plan (Part-2 deliverables):**
  - Dry run: 12–16 Oct.
  - Sprint-1: 19–23 Oct.
  - Sprint-2: 26–30 Oct.
- **Our GitHub Project board:**
  - Iterations are named **Sprint 1** (dry run), **Sprint 2** (= course Sprint-1) and **Sprint 3** (= course Sprint-2).
  - Board Sprint 2 holds compression (#1–#4).
  - Board Sprint 3 holds decompression (#5–#7) and all NFRs (#8–#11).
- **Effect:** the Sprint-1 video shows a working compressor. The full compress → decompress round trip is shown in the Sprint-2 video.
- **STP v1.0 Section 7** listed test execution on 06–07 Oct, before any code existed. Actual testing happens during the sprints. Corrected in STP v1.1.

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
- **Owner:** Pavan (REQ-3). Fix sent to him on 21-Oct-2026. **Status:** 🔧 Fix in progress (PR #__).
- **Retest:** TC-NF-03. Compress on one OS, decompress on another, and compare with `cmp` or `fc /b`.

### D-10: Memory use exceeds the SRS limit (risk)
- **Submitted:** SRS 6.1: "Memory consumption should not exceed the size of the file being processed plus tree overhead."
- **Actual design:**
  - `read_file_bytes()` loads the whole input into memory.
  - `encode()` builds the whole compressed output in memory before writing.
  - Peak memory is therefore about **input size + compressed size + tree**.
- **Indicative measurement (21-Oct, team test run):** compressing a **50 MB** text file used **86 MB** peak memory, about 1.7× the file size. This is above the SRS limit.
- **Action:** measure on a 100 MB file in #10 (Inchara, TC-NF-02).
  - To meet the limit: write the encoded output to the file in chunks instead of keeping it all in memory.
  - Otherwise: record the measured figure here as an accepted deviation.
- **Status:** 🔍 To verify (Sprint 2).

### D-11: 100 MB performance (to be measured officially)
- **Submitted:** SRS 6.1: compress and decompress 100 MB in under 10 s.
- **Indicative measurement (21-Oct):**
  - Compressing a **50 MB** text file took **0.92 s** on a 2-core Intel Xeon 2.8 GHz Linux machine, and the output was 27 MB (46% smaller).
  - So 100 MB should take about 2 s, well within the target.
- **Action:** official measurement on a team laptop in #8 (Pavan, TC-NF-01), for both compression and decompression. Record the CPU and RAM used.
- **Status:** 🔍 To verify (Sprint 2). Compression looks fine; decompression is not built yet.

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
- **Still open:** an existing output file is overwritten without asking, and a write that fails half-way can leave a partial output file. To be handled with REQ-7 (Inchara): write to a temporary file, then rename it when complete.
- **Status:** 🔧 Fix in progress (PR #__).

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
