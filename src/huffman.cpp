
#include "huffman.hpp"

#include <algorithm>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

namespace huffman {

Node::Node(std::uint8_t value, std::uint64_t count)
    : byte(value),
      frequency(count),
      min_byte(value),
      left(nullptr),
      right(nullptr),
      is_leaf(true) {}

Node::Node(std::uint64_t count,
           std::shared_ptr<Node> left_child,
           std::shared_ptr<Node> right_child)
    : byte(0),
      frequency(count),
      min_byte(std::min(left_child->min_byte,
                        right_child->min_byte)),
      left(std::move(left_child)),
      right(std::move(right_child)),
      is_leaf(false) {}

// Build a deterministic Huffman tree using a min-priority queue.
// Order nodes by frequency, then by the smallest byte in the subtree.
std::shared_ptr<Node> build_tree(
    const std::array<std::uint64_t, BYTE_VALUES>& frequencies) {

    auto compare = [](const std::shared_ptr<Node>& a,
                      const std::shared_ptr<Node>& b) {
        if (a->frequency != b->frequency) {
            return a->frequency > b->frequency;
        }

        return a->min_byte > b->min_byte;
    };

    using NodePtr = std::shared_ptr<Node>;

    std::priority_queue<
        NodePtr,
        std::vector<NodePtr>,
        decltype(compare)> queue(compare);

    for (std::size_t i = 0; i < BYTE_VALUES; ++i) {
        if (frequencies[i] > 0) {
            queue.push(std::make_shared<Node>(
                static_cast<std::uint8_t>(i),
                frequencies[i]));
        }
    }

    // Empty input
    if (queue.empty()) {
        return nullptr;
    }

    while (queue.size() > 1) {
        // First node becomes the left child.
        NodePtr left = queue.top();
        queue.pop();

        // Second node becomes the right child.
        NodePtr right = queue.top();
        queue.pop();

        if (left->frequency >
            std::numeric_limits<std::uint64_t>::max()
                - right->frequency) {
            throw std::overflow_error(
                "Huffman frequency overflow");
        }

        const std::uint64_t combined =
            left->frequency + right->frequency;

        queue.push(std::make_shared<Node>(
            combined, left, right));
    }

    return queue.top();
}

// Generate codes recursively
static void generate_codes_recursive(
    const std::shared_ptr<Node>& node,
    const std::string& prefix,
    CodeTable& codes) {

    if (!node) {
        return;
    }

    if (node->is_leaf) {
        // A single-symbol input uses code "0".
        codes[node->byte] = prefix.empty() ? "0" : prefix;
        return;
    }

    generate_codes_recursive(
        node->left, prefix + '0', codes);

    generate_codes_recursive(
        node->right, prefix + '1', codes);
}

CodeTable generate_codes(
    const std::shared_ptr<Node>& root) {

    CodeTable codes{};
    generate_codes_recursive(root, "", codes);

    return codes;
}

// Encode input bytes into a packed bitstream.
// Bits are written most-significant-bit first.
EncodedBitstream encode(
    const std::vector<std::uint8_t>& input,
    const CodeTable& codes) {

    EncodedBitstream result{{}, 0};

    for (std::uint8_t byte : input) {
        const std::string& code = codes[byte];

        if (code.empty()) {
            throw std::invalid_argument(
                "Missing Huffman code for input byte");
        }

        for (char bit : code) {
            if (bit != '0' && bit != '1') {
                throw std::invalid_argument(
                    "Invalid Huffman code");
            }

            if (result.bit_count ==
                std::numeric_limits<std::uint64_t>::max()) {
                throw std::overflow_error(
                    "Encoded bit count overflow");
            }

            // Start a new output byte when needed.
            if (result.bit_count % 8 == 0) {
                result.bytes.push_back(0);
            }

            // Pack bits from most significant to least significant.
            if (bit == '1') {
                const unsigned shift =
                    7u - static_cast<unsigned>(
                        result.bit_count % 8);

                result.bytes.back() |=
                    static_cast<std::uint8_t>(1u << shift);
            }

            ++result.bit_count;
        }
    }

    return result;
}

} // namespace huffman
