// test_robustness.cpp - error-path unit tests (Sprint 3, Part-3 testing)
// Covers the error handling required by SRS 6.2 / SAD 4.4 / STP 5.1:
// damaged archive headers, inconsistent metadata, bad bitstreams, unwritable
// outputs and overflow guards. Each test expects a clean error code and no
// output file. Called from tests/test_main.cpp.
#include "huffzip.h"
#include "huffman.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Bytes = std::vector<std::uint8_t>;

int failures = 0;

void check(bool condition, const std::string& name) {
    if (condition) {
        std::cout << "[PASS] " << name << "\n";
    } else {
        std::cout << "[FAIL] " << name << "\n";
        ++failures;
    }
}

void write_bytes(const std::string& path, const Bytes& data) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
}

Bytes read_bytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return Bytes(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool exists(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return static_cast<bool>(in);
}

void put_u64(Bytes& data, std::size_t offset, std::uint64_t value) {
    std::memcpy(data.data() + offset, &value, sizeof(value));
}

std::uint64_t get_u64(const Bytes& data, std::size_t offset) {
    std::uint64_t value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

// Builds a valid archive of `content` and returns its bytes.
Bytes make_archive(const std::string& content) {
    write_bytes("rb_src.bin", Bytes(content.begin(), content.end()));
    compress_file("rb_src.bin", "rb_src.huff");
    Bytes archive = read_bytes("rb_src.huff");
    std::remove("rb_src.bin");
    std::remove("rb_src.huff");
    return archive;
}

// Writes `archive`, decompresses it, and checks it fails with no output left.
// expected_code < 0 means "any non-zero exit code".
bool rejected(const Bytes& archive, int expected_code = EXIT_FAILURE_OTHER) {
    write_bytes("rb_bad.huff", archive);
    std::remove("rb_out.bin");
    const int code = decompress_file("rb_bad.huff", "rb_out.bin");
    const bool clean = !exists("rb_out.bin") && !exists("rb_out.bin.tmp");
    std::remove("rb_bad.huff");
    std::remove("rb_out.bin");
    std::remove("rb_out.bin.tmp");
    const bool code_ok = expected_code < 0 ? code != EXIT_OK : code == expected_code;
    return code_ok && clean;
}

constexpr std::size_t HEADER = 2064;      // 8 + 256*8 + 8 (see FORMAT.md)
constexpr std::size_t BITCOUNT_AT = 2056;

}  // namespace

void run_robustness_tests() {
    const Bytes good = make_archive("abracadabra");
    const Bytes single = make_archive("AAAAAAAAAA");

    // --- missing / truncated archive headers (REQ-5, TC-ERR-02) ---
    check(decompress_file("rb_no_such_file.huff", "rb_out.bin") == EXIT_FILE_ERROR,
          "UT-ROB-01 missing archive file -> EXIT_FILE_ERROR");
    check(rejected(Bytes(good.begin(), good.begin() + 4)),
          "UT-ROB-02 header cut inside original-size field is rejected");
    check(rejected(Bytes(good.begin(), good.begin() + 500)),
          "UT-ROB-03 header cut inside frequency table is rejected");
    check(rejected(Bytes(good.begin(), good.begin() + 2060)),
          "UT-ROB-04 header cut inside bit-count field is rejected");

    // --- inconsistent metadata (TC-ERR-08) ---
    {
        Bytes bad = good;
        put_u64(bad, 8 + 8 * 1, std::numeric_limits<std::uint64_t>::max());
        put_u64(bad, 8 + 8 * 2, 5);
        check(rejected(bad), "UT-ROB-05 frequency total overflow is rejected");
    }
    {
        Bytes empty = make_archive("");
        put_u64(empty, BITCOUNT_AT, 3);
        check(rejected(empty), "UT-ROB-06 empty archive with non-zero bit count is rejected");
    }
    {
        Bytes bad = good;
        put_u64(bad, BITCOUNT_AT, 0);
        check(rejected(bad), "UT-ROB-07 non-empty archive with zero bit count is rejected");
    }

    // --- single-symbol archives (TC-ERR-04) ---
    {
        Bytes bad = single;
        put_u64(bad, BITCOUNT_AT, 9);  // must equal the original size (10)
        check(rejected(bad), "UT-ROB-08 single-symbol archive with wrong bit count is rejected");
    }
    {
        Bytes bad = single;
        bad[HEADER] = 0x80;  // a '1' bit is impossible for a single symbol
        check(rejected(bad), "UT-ROB-09 single-symbol payload containing a 1-bit is rejected");
    }

    // --- bitstream problems (REQ-6, TC-ERR-02) ---
    {
        Bytes bad = good;
        put_u64(bad, BITCOUNT_AT, get_u64(good, BITCOUNT_AT) - 1);
        check(rejected(bad), "UT-ROB-10 bitstream ending in the middle of a code is rejected");
    }
    {
        Bytes bad = good;
        put_u64(bad, BITCOUNT_AT, get_u64(good, BITCOUNT_AT) + 16);
        bad.push_back(0x00);
        bad.push_back(0x00);
        check(rejected(bad), "UT-ROB-11 extra encoded symbols beyond the original size are rejected");
    }
    {
        Bytes bad = good;
        bad.pop_back();  // payload loses its last byte, bit count unchanged
        check(rejected(bad, EXIT_FILE_ERROR), "UT-ROB-12 payload missing its last byte is rejected (truncated bitstream)");
    }
    {
        Bytes bad = good;
        put_u64(bad, BITCOUNT_AT, get_u64(good, BITCOUNT_AT) + 64);
        check(rejected(bad, -1), "UT-ROB-13 bit count larger than the payload is rejected");
    }

    // --- unreadable input / unwritable output (SRS 6.2, SAD 4.4) ---
    check(compress_file("rb_no_such_input.txt", "rb_x.huff") == EXIT_FILE_ERROR && !exists("rb_x.huff"),
          "UT-ROB-14 compress with unreadable input -> EXIT_FILE_ERROR, no output");
    write_bytes("rb_in.txt", Bytes{'h', 'e', 'l', 'l', 'o'});
    check(compress_file("rb_in.txt", "rb_missing_dir/out.huff") == EXIT_FILE_ERROR,
          "UT-ROB-15 compress into a missing folder -> EXIT_FILE_ERROR");
    write_bytes("rb_ok.huff", good);
    check(decompress_file("rb_ok.huff", "rb_missing_dir/out.bin") == EXIT_FILE_ERROR,
          "UT-ROB-16 decompress into a missing folder -> EXIT_FILE_ERROR");
    std::remove("rb_in.txt");
    std::remove("rb_ok.huff");

    // --- overflow and input guards in the Huffman module (REQ-3) ---
    {
        std::array<std::uint64_t, huffman::BYTE_VALUES> freqs{};
        freqs[0] = std::numeric_limits<std::uint64_t>::max();
        freqs[1] = 1;
        bool threw = false;
        try { huffman::build_tree(freqs); } catch (const std::overflow_error&) { threw = true; }
        check(threw, "UT-ROB-17 build_tree detects frequency overflow");
    }
    {
        huffman::CodeTable codes{};
        codes['x'] = "0x1";
        bool threw = false;
        try { huffman::encode(Bytes{'x'}, codes); } catch (const std::invalid_argument&) { threw = true; }
        check(threw, "UT-ROB-18 encode rejects a code containing characters other than 0/1");
    }

    if (failures != 0) {
        throw std::runtime_error("robustness tests failed");
    }
}
