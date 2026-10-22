// codec.cpp - compression and decompression (REQ-3 to REQ-7)
#include "huffzip.h"
#include "huffman.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <memory>

int compress_file(const std::string& input_path, const std::string& output_path) {
    std::string error;
    std::vector<uint8_t> input_bytes;

    // 1. REQ-1 (Pranavi): Read input file
    if (!read_file_bytes(input_path, input_bytes, error)) {
        std::cerr << "Error reading input: " << error << "\n";
        return EXIT_FILE_ERROR;
    }

    // 2. REQ-2 (Pranavi): Calculate frequencies
    FreqTable freqs = count_frequencies(input_bytes);

    // 3. REQ-3 (Pavan): Build tree, generate codes, encode bitstream
    auto root = huffman::build_tree(freqs);
    auto codes = huffman::generate_codes(root);
    huffman::EncodedBitstream bitstream = huffman::encode(input_bytes, codes);

    // 4. REQ-4 (Inchara): Write binary .huff archive
    std::ofstream out(output_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open output file " << output_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // Write Header: Original File Size (8 bytes)
    uint64_t original_size = input_bytes.size();
    out.write(reinterpret_cast<const char*>(&original_size), sizeof(original_size));

    // Write Header: Frequency Table (256 * 8 = 2048 bytes)
    out.write(reinterpret_cast<const char*>(freqs.data()), sizeof(uint64_t) * 256);

    // Write Header: Exact Bit Count (8 bytes - needed for decoding)
    out.write(reinterpret_cast<const char*>(&bitstream.bit_count), sizeof(bitstream.bit_count));

    // Write Payload: Encoded bitstream bytes
    out.write(reinterpret_cast<const char*>(bitstream.bytes.data()), bitstream.bytes.size());

    out.close();
    if (!out.good()) {
        std::cerr << "Error: Failed to write compressed data properly.\n";
        return EXIT_FILE_ERROR;
    }

    // Calculate compressed size (8 bytes original size + 2048 bytes freq table + 8 bytes bit count + bitstream payload)
    uint64_t compressed_size = 8 + (256 * sizeof(uint64_t)) + 8 + bitstream.bytes.size();
    
    // Calculate compression ratio
    double ratio = 100.0;
    if (original_size > 0) {
        ratio = (static_cast<double>(compressed_size) / original_size) * 100.0;
    }

    std::cout << "Successfully compressed " << input_path << " to " << output_path << "\n";
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

    // Read the 256-entry frequency table.
    FreqTable freqs{};
    in.read(reinterpret_cast<char*>(freqs.data()),
            sizeof(std::uint64_t) * freqs.size());

    if (!in) {
        std::cerr << "Error: Incomplete frequency table.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Read the exact number of meaningful bits.
    std::uint64_t bit_count = 0;
    in.read(reinterpret_cast<char*>(&bit_count),
            sizeof(bit_count));

    if (!in) {
        std::cerr << "Error: Missing bit count.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Validate the metadata before decoding.
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
            std::numeric_limits<std::size_t>::max())) {
        std::cerr << "Error: Original file is too large.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Read the encoded payload.
    std::vector<std::uint8_t> payload;
    char ch;

    while (in.get(ch)) {
        payload.push_back(
            static_cast<std::uint8_t>(
                static_cast<unsigned char>(ch)));
    }

    if (in.bad()) {
        std::cerr << "Error: Failed to read compressed data.\n";
        return EXIT_FILE_ERROR;
    }

    // Check that the payload contains exactly the required bytes.
    const std::uint64_t expected_payload_size =
        bit_count / 8 + (bit_count % 8 != 0 ? 1 : 0);

    if (expected_payload_size != payload.size()) {
        std::cerr << "Error: Invalid or truncated bitstream.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Empty input has no tree and no encoded bits.
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

        // The encoder represents every occurrence using bit '0'.
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
        // Decode exactly bit_count bits, MSB-first.
        auto current = root;

        for (std::uint64_t i = 0; i < bit_count; ++i) {
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

    // Write the recovered file in binary mode.
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
