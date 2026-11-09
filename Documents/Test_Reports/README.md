# Test Reports: Team 14, File Compression Tool (Part-3)

| Document | What it contains |
|---|---|
| [Test_Case_Document_v2.md](Test_Case_Document_v2.md) | Updated test cases (v2.0): changes from the sprints, 26 system/NF cases, 36 unit tests, updated RTM |
| [Unit_Test_Report.md](Unit_Test_Report.md) | 36/36 unit tests passed (GCC, Clang/libc++, sanitizers) |
| [System_Validation_Report.md](System_Validation_Report.md) | 21/21 system cases, 17/17 lossless checks, NF-01/02/03 passed; 1 known limitation (D-16) |
| [Code_Coverage_Report.md](Code_Coverage_Report.md) | Line coverage 89.7%, function coverage 100% |
| [Branch_Coverage_Report.md](Branch_Coverage_Report.md) | Branch coverage 82.3% |
| [coverage/index.html](coverage/index.html) | Detailed per-line HTML coverage report (download and open in a browser) |
| `PDF/` | PDF copies of all the reports above |

**Re-run everything:**
```bash
make clean && make && make test        # unit tests
bash tests/system_validation.sh        # system validation
bash tests/lossless_check.sh           # lossless check (#11)
bash tests/coverage.sh                 # coverage (needs: pip install gcovr)
```
