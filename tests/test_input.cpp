// test_input.cpp - tests for REQ-1 (input validation) and REQ-2 (frequencies)
// Owner: Pranavi Sistla. Called from tests/test_main.cpp.
#include "huffzip.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

void write_file(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary);
    out << content;
}

// TC-COMP-01 (REQ-1): a valid file path is accepted
void tc_comp_01_valid_path() {
    write_file("tc_valid.txt", "hello");
    std::string err;
    assert(validate_input_file("tc_valid.txt", err));
    std::remove("tc_valid.txt");
}

// TC-ERR-01 (REQ-1): a nonexistent or empty path is rejected with a message
void tc_err_01_invalid_path() {
    std::string err;
    assert(!validate_input_file("does_not_exist_12345.txt", err));
    assert(err.find("not found") != std::string::npos);
    assert(!validate_input_file("", err));
}

// TC-COMP-02 (REQ-2): frequencies match a file with known character counts
void tc_comp_02_frequencies() {
    write_file("tc_freq.txt", "aaabbc\n");
    std::vector<std::uint8_t> data;
    std::string err;
    assert(read_file_bytes("tc_freq.txt", data, err));
    assert(data.size() == 7);

    const FreqTable f = count_frequencies(data);
    assert(f['a'] == 3);
    assert(f['b'] == 2);
    assert(f['c'] == 1);
    assert(f['\n'] == 1);
    assert(f['z'] == 0);
    std::remove("tc_freq.txt");
}

// TC-COMP-02 (REQ-2): binary data (all 256 byte values) and an empty input
void tc_comp_02_binary_and_empty() {
    std::vector<std::uint8_t> all;
    for (int i = 0; i < 256; ++i) all.push_back(static_cast<std::uint8_t>(i));
    const FreqTable f = count_frequencies(all);
    for (int i = 0; i < 256; ++i) assert(f[i] == 1);

    const FreqTable empty = count_frequencies({});
    for (int i = 0; i < 256; ++i) assert(empty[i] == 0);
}

}  // namespace

void run_input_tests() {
    tc_comp_01_valid_path();
    tc_err_01_invalid_path();
    tc_comp_02_frequencies();
    tc_comp_02_binary_and_empty();
    std::cout << "[PASS] Input and frequency tests (TC-COMP-01, TC-ERR-01, TC-COMP-02)\n";
}
