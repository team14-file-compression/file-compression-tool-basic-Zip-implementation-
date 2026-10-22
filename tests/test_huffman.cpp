
#include "huffman.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void test_empty_frequencies() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    auto root = huffman::build_tree(frequencies);

    assert(root == nullptr);
}

void test_single_symbol() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 5;

    auto root = huffman::build_tree(frequencies);
    auto codes = huffman::generate_codes(root);

    assert(root != nullptr);
    assert(root->is_leaf);
    assert(codes[static_cast<unsigned char>('A')] == "0");
}

void test_basic_huffman_codes() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 5;
    frequencies[static_cast<unsigned char>('B')] = 2;
    frequencies[static_cast<unsigned char>('C')] = 1;

    auto root = huffman::build_tree(frequencies);
    auto codes = huffman::generate_codes(root);

    assert(root != nullptr);
    assert(!codes[static_cast<unsigned char>('A')].empty());
    assert(!codes[static_cast<unsigned char>('B')].empty());
    assert(!codes[static_cast<unsigned char>('C')].empty());
}

void test_deterministic_tie_breaking() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};

    frequencies[static_cast<unsigned char>('A')] = 1;
    frequencies[static_cast<unsigned char>('B')] = 1;
    frequencies[static_cast<unsigned char>('C')] = 1;
    frequencies[static_cast<unsigned char>('D')] = 1;

    auto root = huffman::build_tree(frequencies);
    auto codes = huffman::generate_codes(root);

    // A and B form the first pair; C and D form the second pair.
    assert(codes[static_cast<unsigned char>('A')] == "00");
    assert(codes[static_cast<unsigned char>('B')] == "01");
    assert(codes[static_cast<unsigned char>('C')] == "10");
    assert(codes[static_cast<unsigned char>('D')] == "11");
}

void test_binary_symbols() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[0] = 2;
    frequencies[255] = 3;

    auto root = huffman::build_tree(frequencies);
    auto codes = huffman::generate_codes(root);

    assert(root != nullptr);
    assert(!codes[0].empty());
    assert(!codes[255].empty());

    std::vector<std::uint8_t> input = {0, 255, 0, 255, 255};
    auto encoded = huffman::encode(input, codes);

    assert(encoded.bit_count > 0);
    assert(encoded.bytes.size() == (encoded.bit_count + 7) / 8);
}

void test_encoded_size_consistency() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('X')] = 3;
    frequencies[static_cast<unsigned char>('Y')] = 2;

    auto root = huffman::build_tree(frequencies);
    auto codes = huffman::generate_codes(root);

    std::vector<std::uint8_t> input = {'X', 'Y', 'X', 'Y', 'X'};
    auto encoded = huffman::encode(input, codes);

    assert(encoded.bytes.size() == (encoded.bit_count + 7) / 8);
}

void test_missing_code_throws() {
    huffman::CodeTable codes{};
    std::vector<std::uint8_t> input = {'A'};

    bool threw = false;

    try {
        (void)huffman::encode(input, codes);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
}

} // namespace

void run_huffman_tests() {
    test_empty_frequencies();
    test_single_symbol();
    test_basic_huffman_codes();
    test_deterministic_tie_breaking();
    test_binary_symbols();
    test_encoded_size_consistency();
    test_missing_code_throws();

    std::cout << "[PASS] Huffman tests\n";
}
