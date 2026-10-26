
#include "huffzip.h"
#include "huffman.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <memory>
#include <cstdint>
#include <exception>
#include <array>

int compress_file(const std::string& input_path,
                  const std::string& output_path) {
    std::string error;
    std::vector<uint8_t> input_bytes;

    // REQ-1: Read input file.
    if (!read_file_bytes(input_path, input_bytes, error)) {
        std::cerr << "Error reading input: " << error << "\n";
        return EXIT_FILE_ERROR;
    }

    // REQ-2: Calculate frequencies.
    FreqTable freqs = count_frequencies(input_bytes);

    // REQ-3: Build tree, generate codes, encode bitstream.
    auto root = huffman::build_tree(freqs);
    auto codes = huffman::generate_codes(root);
    huffman::EncodedBitstream bitstream =
        huffman::encode(input_bytes, codes);

    // REQ-4: Write binary .huff archive.
    std::ofstream out(output_path, std::ios::binary);

    if (!out) {
        std::cerr << "Error: Could not open output file "
                  << output_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // Original file size: 8 bytes.
    const std::uint64_t original_size = input_bytes.size();

    out.write(reinterpret_cast<const char*>(&original_size),
              sizeof(original_size));

    // Frequency table: 256 entries.
    out.write(reinterpret_cast<const char*>(freqs.data()),
              sizeof(std::uint64_t) * freqs.size());

    // Exact encoded bit count: 8 bytes.
    out.write(reinterpret_cast<const char*>(&bitstream.bit_count),
              sizeof(bitstream.bit_count));

    // Encoded payload.
    if (!bitstream.bytes.empty()) {
        out.write(
            reinterpret_cast<const char*>(bitstream.bytes.data()),
            static_cast<std::streamsize>(bitstream.bytes.size()));
    }

    out.close();

    if (!out.good()) {
        std::cerr << "Error: Failed to write compressed data properly.\n";
        return EXIT_FILE_ERROR;
    }

    const std::uint64_t compressed_size =
        8 + sizeof(std::uint64_t) * freqs.size()
        + 8 + bitstream.bytes.size();

    double ratio = 100.0;

    if (original_size > 0) {
        ratio = (static_cast<double>(compressed_size) /
                 static_cast<double>(original_size)) * 100.0;
    }

    std::cout << "Successfully compressed " << input_path
              << " to " << output_path << "\n";
    std::cout << "Original size:   " << original_size << " bytes\n";
    std::cout << "Compressed size: " << compressed_size << " bytes\n";
    std::cout << "Ratio:           " << ratio << "%\n";

    return EXIT_OK;
}

int decompress_file(const std::string& input_path,
                    const std::string& output_path) {
    std::ifstream in(input_path, std::ios::binary);

    if (!in) {
        std::cerr << "Error: Could not open compressed file "
                  << input_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // REQ-5: Read original size.
    std::uint64_t original_size = 0;

    in.read(reinterpret_cast<char*>(&original_size),
            sizeof(original_size));

    if (!in) {
        std::cerr << "Error: Truncated archive header.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Read frequency table.
    FreqTable freqs{};

    in.read(reinterpret_cast<char*>(freqs.data()),
            sizeof(std::uint64_t) * freqs.size());

    if (!in) {
        std::cerr << "Error: Incomplete frequency table.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Read exact number of meaningful bits.
    std::uint64_t bit_count = 0;

    in.read(reinterpret_cast<char*>(&bit_count),
            sizeof(bit_count));

    if (!in) {
        std::cerr << "Error: Missing bit count.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Validate frequency metadata and check overflow.
    std::uint64_t total_frequency = 0;

    for (std::uint64_t frequency : freqs) {
        if (frequency >
            std::numeric_limits<std::uint64_t>::max()
                - total_frequency) {
            std::cerr << "Error: Frequency total overflow.\n";
            return EXIT_FAILURE_OTHER;
        }

        total_frequency += frequency;
    }

    if (total_frequency != original_size) {
        std::cerr << "Error: Invalid original size metadata.\n";
        return EXIT_FAILURE_OTHER;
    }

    if (original_size >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max()) ||
        original_size >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::streamsize>::max())) {
        std::cerr << "Error: Original file is too large.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Calculate expected payload size without overflowing bit_count + 7.
    const std::uint64_t expected_payload_size =
        bit_count / 8 + (bit_count % 8 != 0 ? 1 : 0);

    if (expected_payload_size >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max()) ||
        expected_payload_size >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::streamsize>::max())) {
        std::cerr << "Error: Payload is too large.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Read the entire payload using buffered I/O.
    std::vector<std::uint8_t> payload(
        static_cast<std::size_t>(expected_payload_size));

    if (!payload.empty()) {
        in.read(reinterpret_cast<char*>(payload.data()),
                static_cast<std::streamsize>(payload.size()));

        if (!in) {
            std::cerr << "Error: Truncated or unreadable payload.\n";
            return EXIT_FAILURE_OTHER;
        }
    }

    // Reject unexpected trailing data.
    char extra;

    if (in.get(extra)) {
        std::cerr << "Error: Unexpected trailing data in archive.\n";
        return EXIT_FAILURE_OTHER;
    }

    if (in.bad()) {
        std::cerr << "Error: Failed to read compressed data.\n";
        return EXIT_FILE_ERROR;
    }

    // Validate empty and non-empty file metadata.
    if (original_size == 0 && bit_count != 0) {
        std::cerr << "Error: Invalid empty-file bit count.\n";
        return EXIT_FAILURE_OTHER;
    }

    if (original_size != 0 && bit_count == 0) {
        std::cerr << "Error: Missing encoded bits.\n";
        return EXIT_FAILURE_OTHER;
    }

    // REQ-6: Rebuild the deterministic Huffman tree.
    std::shared_ptr<huffman::Node> root;

    try {
        root = huffman::build_tree(freqs);
    } catch (const std::exception& e) {
        std::cerr << "Error rebuilding Huffman tree: "
                  << e.what() << "\n";
        return EXIT_FAILURE_OTHER;
    }

    if (original_size != 0 && !root) {
        std::cerr << "Error: Missing Huffman tree.\n";
        return EXIT_FAILURE_OTHER;
    }

    std::vector<std::uint8_t> decoded;
    decoded.reserve(static_cast<std::size_t>(original_size));

    // Special case: a tree containing only one symbol.
    if (root && root->is_leaf) {
        if (root->frequency != original_size ||
            bit_count != original_size) {
            std::cerr << "Error: Invalid single-symbol archive.\n";
            return EXIT_FAILURE_OTHER;
        }

        for (std::uint64_t i = 0; i < bit_count; ++i) {
            const unsigned shift =
                7u - static_cast<unsigned>(i % 8);

            if (((payload[static_cast<std::size_t>(i / 8)]
                  >> shift) & 1u) != 0) {
                std::cerr << "Error: Invalid single-symbol bitstream.\n";
                return EXIT_FAILURE_OTHER;
            }

            decoded.push_back(root->byte);
        }
    } else if (root) {
        // Build a lookup table for prefixes of up to 8 bits.
        // Entries are used only when a complete symbol is found
        // within the next 8 bits. Longer codes use the fallback.
        struct DecodeEntry {
            std::uint8_t byte = 0;
            unsigned bits_used = 0;
            bool valid = false;
        };

        constexpr unsigned LOOKUP_BITS = 8;
        constexpr unsigned LOOKUP_SIZE = 1u << LOOKUP_BITS;

        std::array<DecodeEntry, LOOKUP_SIZE> table{};

        for (unsigned prefix = 0; prefix < LOOKUP_SIZE; ++prefix) {
            auto node = root;

            for (unsigned depth = 1; depth <= LOOKUP_BITS; ++depth) {
                const unsigned shift = LOOKUP_BITS - depth;
                const bool bit = ((prefix >> shift) & 1u) != 0;

                node = bit ? node->right : node->left;

                if (!node) {
                    break;
                }

                if (node->is_leaf) {
                    table[prefix].byte = node->byte;
                    table[prefix].bits_used = depth;
                    table[prefix].valid = true;
                    break;
                }
            }
        }

        auto current = root;
        std::uint64_t i = 0;

        while (i < bit_count) {
            const std::uint64_t remaining = bit_count - i;

            // Fast path: use the table at byte boundaries when
            // at least 8 meaningful bits remain.
            if (current == root &&
                (i % 8 == 0) &&
                remaining >= LOOKUP_BITS) {
                const std::size_t byte_index =
                    static_cast<std::size_t>(i / 8);

                const unsigned prefix = payload[byte_index];
                const DecodeEntry& entry = table[prefix];

                if (entry.valid) {
                    decoded.push_back(entry.byte);
                    i += entry.bits_used;

                    if (decoded.size() > original_size) {
                        std::cerr << "Error: Decoded data exceeds "
                                     "original size.\n";
                        return EXIT_FAILURE_OTHER;
                    }

                    continue;
                }
            }

            // Safe fallback: traverse the tree one bit at a time.
            const std::uint8_t byte =
                payload[static_cast<std::size_t>(i / 8)];

            const unsigned shift =
                7u - static_cast<unsigned>(i % 8);

            const bool bit = ((byte >> shift) & 1u) != 0;

            current = bit ? current->right : current->left;

            if (!current) {
                std::cerr << "Error: Invalid Huffman bitstream.\n";
                return EXIT_FAILURE_OTHER;
            }

            ++i;

            if (current->is_leaf) {
                decoded.push_back(current->byte);
                current = root;

                if (decoded.size() > original_size) {
                    std::cerr << "Error: Decoded data exceeds "
                                 "original size.\n";
                    return EXIT_FAILURE_OTHER;
                }
            }
        }

        // The final bit must end at a symbol boundary.
        if (current != root) {
            std::cerr << "Error: Incomplete Huffman code.\n";
            return EXIT_FAILURE_OTHER;
        }
    }

    if (decoded.size() != original_size) {
        std::cerr << "Error: Decoded size does not match "
                     "original size.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Write recovered file.
    std::ofstream out(output_path, std::ios::binary);

    if (!out) {
        std::cerr << "Error: Could not open output file "
                  << output_path << "\n";
        return EXIT_FILE_ERROR;
    }

    if (!decoded.empty()) {
        out.write(reinterpret_cast<const char*>(decoded.data()),
                  static_cast<std::streamsize>(decoded.size()));
    }

    out.close();

    if (!out) {
        std::cerr << "Error: Failed to write decompressed file.\n";
        return EXIT_FILE_ERROR;
    }

    std::cout << "Successfully decompressed "
              << input_path << " to " << output_path << "\n";
    std::cout << "Decompressed size: "
              << decoded.size() << " bytes\n";

    return EXIT_OK;
}
