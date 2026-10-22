
#ifndef HUFFMAN_HPP
#define HUFFMAN_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace huffman {

constexpr std::size_t BYTE_VALUES = 256;

// Huffman tree node
struct Node {
    std::uint8_t byte;
    std::uint64_t frequency;

    // Smallest byte value in this node's subtree.
    // Used to break ties deterministically.
    std::uint8_t min_byte;

    std::shared_ptr<Node> left;
    std::shared_ptr<Node> right;

    bool is_leaf;

    // Leaf node
    Node(std::uint8_t value, std::uint64_t count);

    // Internal node
    Node(std::uint64_t count,
         std::shared_ptr<Node> left_child,
         std::shared_ptr<Node> right_child);
};

// One binary code for each possible byte value
using CodeTable = std::array<std::string, BYTE_VALUES>;

// Packed encoded data and number of meaningful bits
struct EncodedBitstream {
    std::vector<std::uint8_t> bytes;
    std::uint64_t bit_count;
};

// Build a deterministic Huffman tree from byte frequencies
std::shared_ptr<Node> build_tree(
    const std::array<std::uint64_t, BYTE_VALUES>& frequencies);

// Generate Huffman codes for the tree
CodeTable generate_codes(
    const std::shared_ptr<Node>& root);

// Encode input bytes using the generated code table
EncodedBitstream encode(
    const std::vector<std::uint8_t>& input,
    const CodeTable& codes);

} // namespace huffman

#endif
