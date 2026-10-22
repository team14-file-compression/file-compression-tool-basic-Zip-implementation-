# `.huff` File Format Specification

**Project:** File Compression Tool (Basic ZIP Implementation): Team 14
**Owner:** Pranavi Sistla (PES1UG24CS338)
**Version:** 1.0 (21-Oct-2026)
**Status:** Matches the compressor merged in PR #16 (REQ-4). The decompressor (REQ-5, REQ-6, REQ-7) must read exactly this format.

| Version | Date | Change |
|---|---|---|
| 1.0 | 21-Oct-2026 | First version, written from `src/codec.cpp` (PR #16) and `src/huffman.cpp` (PR #12) |

---

## 1. Purpose

This document defines the exact byte layout of a compressed `.huff` file. The compressor (`compress_file()`, REQ-4) writes this layout, and the decompressor (`decompress_file()`, REQ-5 to REQ-7) reads it. If the two sides disagree on even one byte, decompression is not lossless.

It expands **SRS Appendix B (Field Layouts)**, which lists the fields but not their exact sizes or order. Differences from Appendix B are recorded in `DEVIATIONS.md` (D-05).

> This is a custom format. It is **not** compatible with standard `.zip` archives (SAD 1.4, SAD 4.1).

---

## 2. Layout at a glance

```
offset 0            8                              2056          2064
       +------------+------------------------------+-------------+--------------------+
       | original   | frequency table              | bit count   | encoded bitstream  |
       | size       | 256 x uint64                 |             | (payload)          |
       | 8 bytes    | 2048 bytes                   | 8 bytes     | ceil(bits/8) bytes |
       +------------+------------------------------+-------------+--------------------+
       |<------------------- header: 2064 bytes ------------------->|
```

| # | Field | Offset (bytes) | Size (bytes) | Type | Description |
|---|---|---|---|---|---|
| 1 | Original size | 0 | 8 | `uint64` | Number of bytes in the original (uncompressed) file. |
| 2 | Frequency table | 8 | 2048 | 256 × `uint64` | Entry *i* = how many times byte value *i* (0–255) appears in the original file. Entry for byte *i* is at offset `8 + 8 × i`. |
| 3 | Bit count | 2056 | 8 | `uint64` | Exact number of meaningful bits in the payload. |
| 4 | Encoded bitstream | 2064 | `ceil(bit count / 8)` | bytes | Huffman-encoded data (see Section 4). |

**Total file size** = `2064 + ceil(bit count / 8)` bytes.

---

## 3. Byte order

All `uint64` fields are written in **little-endian** order (least significant byte first).

The code writes the values directly from memory (`out.write(reinterpret_cast<const char*>(&value), 8)`). That gives little-endian on every machine the team and GitHub Actions use (Intel/AMD x86-64 and Apple Silicon ARM64). Big-endian machines are out of scope.

---

## 4. Encoded bitstream

1. Each input byte is replaced by its Huffman code (a string of `0`s and `1`s).
2. Codes are written one after another with no separators.
3. Bits are packed **most-significant bit first**: the first bit goes into bit 7 (value 128) of the first payload byte, the second into bit 6, and so on.
4. If the last byte is not full, its unused low bits are `0` (padding). The decoder must stop after exactly **bit count** bits and ignore the padding.

---

## 5. How the Huffman codes are built

The decoder does **not** read the codes from the file. It rebuilds the same tree from the frequency table, so both sides **must build the tree in exactly the same way**:

1. Create one leaf for every byte value whose frequency is greater than 0.
2. Repeatedly take the two nodes with the **lowest frequency** out of a min-priority queue, and join them under a new parent whose frequency is their sum. The first node taken becomes the **left** child, the second the **right** child.
3. **Tie-break rule:** when two nodes have the same frequency, the node containing the **smaller byte value** comes out first. An internal node counts as the smallest byte value anywhere in its subtree.
4. Going **left adds `0`**, going **right adds `1`**.
5. **Single-symbol file** (only one distinct byte, e.g. `AAAAA`): the tree is a single leaf and that byte gets the code `0`.
6. **Empty file**: no tree. Original size = 0, all frequencies = 0, bit count = 0, no payload. The file is exactly 2064 bytes.

> ⚠️ **Rule 3 is not in `main` yet.** The current `src/huffman.cpp` (PR #12) compares frequency only. On a tie, the order then depends on the C++ standard library. **We tested this:** the same frequencies give *different* codes with GCC's libstdc++ (Linux and Windows/MinGW) and Apple's libc++ (macOS). A file compressed on Windows would decode wrongly on a Mac. A fix has been sent to the REQ-3 owner; see `DEVIATIONS.md` (D-09). Until it is merged, compress and decompress on the same OS.

---

## 6. How to decompress (for REQ-5, REQ-6, REQ-7)

1. Open the file in **binary** mode.
2. **Validate the header (REQ-5, SRS 6.2):**
   - The file must be at least 2064 bytes long.
   - Read the original size, the 256 frequencies and the bit count.
   - The sum of all 256 frequencies must equal the original size.
   - The payload length must equal `ceil(bit count / 8)`.
   - If any check fails: print an error, write nothing, return `EXIT_FAILURE_OTHER` (3). This is test case TC-ERR-02.
3. Rebuild the tree with `huffman::build_tree(frequencies)`, following Section 5.
4. **Decode (REQ-6):** start at the root. For each of the first *bit count* bits, go left on `0` and right on `1`. On reaching a leaf, output its byte and go back to the root. For a single-leaf tree, each `0` bit outputs that byte.
5. **Check:** the number of bytes produced must equal the original size. Otherwise report a corrupted file.
6. **Write (REQ-7):** write the bytes to the output file in binary mode. Only create or replace the output file after decoding succeeds, so a failure never damages an existing file (SRS 6.2).

---

## 7. Worked example

Input file `ex.txt` containing the 11 bytes `abracadabra`.

**Frequencies:** `a`=5, `b`=2, `r`=2, `c`=1, `d`=1

**Codes** (using the tie-break rule in Section 5):

| Byte | Frequency | Code |
|---|---|---|
| `a` (0x61) | 5 | `0` |
| `r` (0x72) | 2 | `10` |
| `b` (0x62) | 2 | `110` |
| `c` (0x63) | 1 | `1110` |
| `d` (0x64) | 1 | `1111` |

**Bitstream:** `a b r a c a d a b r a` → `0 110 10 0 1110 0 1111 0 110 10 0` = 23 bits

**Packed payload:** `01101001 11001111 0110100`**`0`** → `69 CF 68` (the final `0` is padding)

**Resulting file** (2067 bytes):

| Offset | Bytes (hex) | Meaning |
|---|---|---|
| 0 | `0B 00 00 00 00 00 00 00` | original size = 11 |
| 784 | `05 00 00 00 00 00 00 00` | frequency of `a` (0x61 → 8 + 8×97) = 5 |
| 792 | `02 00 00 00 00 00 00 00` | frequency of `b` = 2 |
| 800 | `01 00 00 00 00 00 00 00` | frequency of `c` = 1 |
| 808 | `01 00 00 00 00 00 00 00` | frequency of `d` = 1 |
| 920 | `02 00 00 00 00 00 00 00` | frequency of `r` = 2 |
| (others in 8–2055) | `00 …` | all other frequencies = 0 |
| 2056 | `17 00 00 00 00 00 00 00` | bit count = 23 |
| 2064 | `69 CF 68` | payload |

You can check a real file on a Mac with: `xxd -s 2056 ex.huff`

---

## 8. Size behaviour (expected, not a bug)

- The header is always **2064 bytes**, so files smaller than about 2–3 KB get **bigger** after compression.
- Text files shrink well. A 1.3 MB text file (numbers 1–200000) compressed to 0.56 MB, a 57% reduction.
- Random or already-compressed data (`.zip`, `.jpg`, `.mp4`) does not shrink. The output is the input size plus 2064 bytes.

These are listed in `DEVIATIONS.md` (D-07) so they can be explained at the demo.

---

## 9. Where this is implemented

| Part | File | Owner |
|---|---|---|
| Reading input, frequency table | `src/file_input.cpp`, `src/frequency.cpp` | Pranavi (REQ-1, REQ-2) |
| Tree, codes, bit packing | `src/huffman.cpp`, `include/huffman.hpp` | Pavan (REQ-3) |
| Writing the `.huff` file | `src/codec.cpp` → `compress_file()` | Inchara (REQ-4) |
| Reading and decoding the `.huff` file | `src/codec.cpp` → `decompress_file()` | Rachit (REQ-5, REQ-6) |
| Writing the restored file | `src/codec.cpp` → `decompress_file()` | Inchara (REQ-7) |

**Any change to this format must update this document in the same PR.**
