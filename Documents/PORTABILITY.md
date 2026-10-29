# Portability Test Results: Issue #9 / TC-NF-03

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Owner:** Pranavi Sistla (PES1UG24CS338)
**Requirement:** SRS 2.4 (runs on Windows, Linux, macOS) and SRS 6.4 (portability and lossless correctness)
**Test case:** STP TC-NF-03: *Build and run the basic test on each available target OS → build and core behaviour work on each tested OS.*
**Date:** 29-Oct-2026
**Result:** ✅ **PASS**

---

## 1. Automated builds on three operating systems (GitHub Actions)

Every push and pull request to `main` runs `.github/workflows/build.yml` on three runners:

| OS (runner) | Compiler | Build `huffzip` | Unit tests (`run_tests`) | CLI smoke test* |
|---|---|---|---|---|
| Ubuntu (`ubuntu-latest`) | GCC (libstdc++) | ✅ | ✅ | ✅ |
| Windows (`windows-latest`) | MinGW-w64 GCC | ✅ | ✅ | ✅ |
| macOS (`macos-latest`) | Apple Clang (libc++) | ✅ | ✅ | ✅ |

\*Smoke test: `-h` works, a missing file is rejected, a file compresses, and overwriting the input is refused.

Tick each box from the Actions run. **Evidence:** the green run for the latest commit on `main`. Link: https://github.com/team14-file-compression/file-compression-tool-basic-Zip-implementation-/actions/runs/40862046404 (run #46, commit bcab100, all 3 jobs passed). Screenshot: attached to the pull request.

---

## 2. Cross-compiler round trips (format compatibility)

The decompressor rebuilds the Huffman tree from the stored frequency table, so the tree must be identical on every compiler. Before PR #24, ties were broken differently by GCC's libstdc++ (Linux/Windows) and Apple's libc++ (macOS), so archives did not decode correctly across systems (see `DEVIATIONS.md`, D-09).

**Retest on the latest code (28-Oct-2026, `main` @ `2895a8a`, after PR #24, #28, #30 and #31):** each file was compressed with one build and decompressed with the other, then compared byte for byte with `cmp`.

| Test file | Size | GCC → Clang/libc++ | Clang/libc++ → GCC |
|---|---|---|---|
| `abracadabra` text | 11 B | ✅ identical | ✅ identical |
| Empty file | 0 B | ✅ | ✅ |
| One repeated byte (`AAAAAAAAAA`) | 10 B | ✅ | ✅ |
| Single byte `a` | 1 B | ✅ | ✅ |
| Two bytes `ab` | 2 B | ✅ | ✅ |
| Random binary | 300 KB | ✅ | ✅ |
| Numbers 1–300000 (text) | 2 MB | ✅ | ✅ |
| All 256 byte values | 5 KB | ✅ | ✅ |
| Skewed binary distribution | 200 KB | ✅ | ✅ |
| Source file `src/codec.cpp` | 12 KB | ✅ | ✅ |
| Windows-style text (CRLF line endings) | 2 KB | ✅ | ✅ |

**22 of 22 cross-compiler round trips are identical.** The same files also passed under AddressSanitizer and UndefinedBehaviorSanitizer with no errors.

---

## 3. Real machines: Mac ↔ Windows exchange (27–29 Oct 2026)

Each archive is decompressed on the other OS and compared with the **sender's own copy** of the input file. This matters because Git for Windows checks text files out with CRLF line endings, so `README.md` is 2128 bytes on Windows and 2070 bytes on the Mac (see `DEVIATIONS.md`, D-18).

| Machine | OS / CPU | Compiler |
|---|---|---|
| Pranavi | macOS, Apple Silicon (arm64) | Apple clang 21.0.0 (clang-2100.1.1.101) |
| Pavan | Windows 11, Intel Core i5-12450H | MSYS2 g++ 16.2.0 |

| Step | Machine | Command | Result |
|---|---|---|---|
| 1 | macOS | `./huffzip -c README.md -o mac.huff` (2070 → 3298 bytes) and send `mac.huff` with `README_mac.md` | ✅ |
| 2 | Windows | `.\huffzip.exe -d mac.huff -o README_from_mac.md` then `fc /b README_mac.md README_from_mac.md` | ✅ "FC: no differences encountered" (2070 bytes) |
| 3 | Windows | `.\huffzip.exe -c README.md -o win.huff` (2128 → 3346 bytes) and send `win.huff` with `README_win.md` | ✅ |
| 4 | macOS | `./huffzip -d win.huff -o README_from_win.md` then `cmp README_win.md README_from_win.md` | ✅ identical (2128 bytes, including the 58 Windows line endings) |
| 5 | Windows | Run step 3 a second time onto the existing `win.huff` (checks that the temp-file rename replaces an existing file on Windows, PR #31) | ✅ "Successfully compressed" both times |
| 6 | Windows | `.\huffzip.exe -d win.huff -o README_from_win.md` then `fc /b README.md README_from_win.md` (same-OS check) | ✅ "FC: no differences encountered" |

**Evidence:** screenshots attached to the pull request that closes #9 (Mac Terminal: steps 1 and 4; Windows VS Code terminal: steps 2, 3, 5 and 6).

---

## 4. Portability notes
- All files are opened in **binary mode**, so Windows does not convert `\n` to `\r\n`.
- Header numbers are stored **little-endian**, which is native on Intel/AMD and Apple Silicon (see `FORMAT.md` Section 3). Big-endian machines are out of scope.
- Built with `-std=c++17` and only the standard library. No OS-specific code.

## 5. Conclusion
The tool builds, passes all unit tests and restores files byte for byte on Windows, Linux and macOS. Archives created on macOS (Apple clang, arm64) decompress identically on Windows (g++ 16.2.0, x86-64) and the other way round. Replacing an existing output file also works on Windows. **TC-NF-03: PASS.**
