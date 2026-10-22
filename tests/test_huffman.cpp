
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

    const auto codes = huffman::generate_codes(root);
    for (const auto& code : codes) {
        assert(code.empty());
    }
}

void test_single_symbol_repeated() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 5;

    auto root = huffman::build_tree(frequencies);
    assert(root != nullptr);
    assert(root->is_leaf);

    const auto codes = huffman::generate_codes(root);
    assert(codes[static_cast<unsigned char>('A')] == "0");

    const std::vector<std::uint8_t> input(5, 'A');
    const auto encoded = huffman::encode(input, codes);

    assert(encoded.bit_count == 5);
    assert(encoded.bytes.size() == 1);
    assert(encoded.bytes[0] == 0);
}

void test_multiple_symbols_and_encoding() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('A')] = 5;
    frequencies[static_cast<unsigned char>('B')] = 2;
    frequencies[static_cast<unsigned char>('C')] = 1;

    auto root = huffman::build_tree(frequencies);
    assert(root != nullptr);

    const auto codes = huffman::generate_codes(root);

    assert(!codes[static_cast<unsigned char>('A')].empty());
    assert(!codes[static_cast<unsigned char>('B')].empty());
    assert(!codes[static_cast<unsigned char>('C')].empty());

    const std::vector<std::uint8_t> input = {'A', 'B', 'C', 'A'};
    const auto encoded = huffman::encode(input, codes);

    assert(encoded.bit_count > 0);
    assert(encoded.bytes.size() == (encoded.bit_count + 7) / 8);
}

void test_binary_symbols() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[0] = 2;
    frequencies[255] = 3;

    auto root = huffman::build_tree(frequencies);
    assert(root != nullptr);

    const auto codes = huffman::generate_codes(root);

    assert(!codes[0].empty());
    assert(!codes[255].empty());
    assert(codes[0] != codes[255]);

    const std::vector<std::uint8_t> input = {0, 255, 0, 255, 255};
    const auto encoded = huffman::encode(input, codes);

    assert(encoded.bit_count > 0);
    assert(encoded.bytes.size() == (encoded.bit_count + 7) / 8);
}

void test_encoded_size_consistency() {
    std::array<std::uint64_t, huffman::BYTE_VALUES> frequencies{};
    frequencies[static_cast<unsigned char>('X')] = 4;
    frequencies[static_cast<unsigned char>('Y')] = 3;
    frequencies[static_cast<unsigned char>('Z')] = 2;

    auto root = huffman::build_tree(frequencies);
    assert(root != nullptr);

    const auto codes = huffman::generate_codes(root);

    const std::vector<std::uint8_t> input = {
        'X', 'Y', 'Z', 'X', 'X', 'Y', 'Z', 'X', 'Y'
    };
    const auto encoded = huffman::encode(input, codes);

    std::uint64_t expected_bits = 0;
    for (const auto byte : input) {
        expected_bits += codes[byte].size();
    }

    assert(encoded.bit_count == expected_bits);
    assert(encoded.bytes.size() == (expected_bits + 7) / 8);
}

void test_missing_code_throws() {
    huffman::CodeTable codes{};
    const std::vector<std::uint8_t> input = {'A'};

    bool threw = false;

    try {
        (void)huffman::encode(input, codes);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    assert(threw);
}

} // namespace

// Called by the shared project test runner.
void run_huffman_tests() {
    test_empty_frequencies();
    test_single_symbol_repeated();
    test_multiple_symbols_and_encoding();
    test_binary_symbols();
    test_encoded_size_consistency();
    test_missing_code_throws();

    std::cout << "[PASS] Huffman tests\n";
}
