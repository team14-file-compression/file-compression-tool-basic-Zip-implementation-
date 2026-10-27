# TC-NF-01 - Sprint 2 Performance Test Results

## Project
File Compression Tool - Team 14

## Test Objective
Verify that compression and decompression of a 100 MiB file each complete in under 10 seconds.

## Test Environment
- Operating System: Windows
- CPU: 12th Gen Intel Core i5-12450H
- Installed RAM: 15.70 GiB
- Compiler: MSYS2 g++ 16.2.0
- Executable: huffzip.exe
- Executable last modified: 26 October 2026, 10:59:08 AM

## Test Data
- Input: sprint2_test_varied_100mb.bin
- File size: 104,857,600 bytes (100 MiB)
- Algorithm: Huffman coding

## Benchmark Results

| Run | Compression (s) | Decompression (s) |
|-----|-----------------|-------------------|
| 1 | 4.3399290 | 0.5765805 |
| 2 | 4.2221430 | 0.5552204 |
| 3 | 4.5006184 | 0.5828755 |
| Average | 4.3542301 | 0.5715588 |

## Lossless Verification
All three decompressed files were compared against the original using fc.exe /b.

- Run 1: PASS - no differences encountered
- Run 2: PASS - no differences encountered
- Run 3: PASS - no differences encountered

## Acceptance Criteria
- Compression under 10 seconds: PASS
- Decompression under 10 seconds: PASS
- Byte-for-byte restoration: PASS

## Final Result
TC-NF-01: PASS

All three runs satisfied the performance requirement.
All restored files matched the original exactly.

## Notes
Timings were measured using PowerShell Measure-Command.
The benchmark input and generated binary outputs are kept locally
and are not committed to GitHub.
