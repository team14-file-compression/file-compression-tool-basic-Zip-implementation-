// codec.cpp - compression and decompression (REQ-3 to REQ-7)
#include "huffzip.h"
#include "huffman.hpp"
#include <filesystem>
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

    // 3. REQ-3 (Pavan): Build tree and generate codes
    auto root = huffman::build_tree(freqs);
    auto codes = huffman::generate_codes(root);

    // [NFR #10] Memory Optimization: Pre-calculate the exact bit count to avoid allocating
    // an enormous EncodedBitstream vector. Peak memory stays strictly at (Input Size + Tree).
    uint64_t bit_count = 0;
    for (size_t i = 0; i < 256; ++i) {
        bit_count += freqs[i] * codes[i].length();
    }

    // [REQ-7 & D-14] Safety: Write to a temporary file first, so we never corrupt existing data
    std::string temp_path = output_path + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open output file " << temp_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // 4. REQ-4 (Inchara): Write Header
    uint64_t original_size = input_bytes.size();
    out.write(reinterpret_cast<const char*>(&original_size), sizeof(original_size));
    out.write(reinterpret_cast<const char*>(freqs.data()), sizeof(uint64_t) * 256);
    out.write(reinterpret_cast<const char*>(&bit_count), sizeof(bit_count));

    // [NFR #10] Stream encoded bits directly to disk instead of holding them in memory
    uint8_t buffer = 0;
    unsigned bit_pos = 0;
    for (uint8_t byte : input_bytes) {
        const std::string& code = codes[byte];
        for (char bit : code) {
            if (bit == '1') {
                buffer |= (1u << (7 - bit_pos));
            }
            bit_pos++;
            if (bit_pos == 8) {
                out.put(static_cast<char>(buffer));
                buffer = 0;
                bit_pos = 0;
            }
        }
    }
    if (bit_pos > 0) {
        out.put(static_cast<char>(buffer));
    }

    out.close();
    if (!out.good()) {
        std::cerr << "Error: Failed to write compressed data properly.\n";
        std::filesystem::remove(temp_path);
        return EXIT_FILE_ERROR;
    }

    // [REQ-7 & D-14] Success: safely rename temp file to final output path
    std::error_code ec;
    std::filesystem::rename(temp_path, output_path, ec);
    if (ec) {
        std::cerr << "Error renaming temp file: " << ec.message() << "\n";
        std::filesystem::remove(temp_path);
        return EXIT_FILE_ERROR;
    }

    // Calculate and display metrics
    uint64_t compressed_size = 8 + (256 * sizeof(uint64_t)) + 8 + (bit_count / 8 + (bit_count % 8 != 0 ? 1 : 0));
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

int decompress_file(const std::string& input_path, const std::string& output_path) {
    std::ifstream in(input_path, std::ios::binary);
    if (!in) {
        std::cerr << "Error: Could not open compressed file " << input_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // REQ-5: Read metadata
    std::uint64_t original_size = 0;
    in.read(reinterpret_cast<char*>(&original_size), sizeof(original_size));

    FreqTable freqs{};
    in.read(reinterpret_cast<char*>(freqs.data()), sizeof(std::uint64_t) * freqs.size());

    std::uint64_t bit_count = 0;
    in.read(reinterpret_cast<char*>(&bit_count), sizeof(bit_count));

    if (!in) {
        std::cerr << "Error: Truncated archive header.\n";
        return EXIT_FAILURE_OTHER;
    }

    std::uint64_t total_frequency = 0;
    for (std::uint64_t frequency : freqs) {
        total_frequency += frequency;
    }
    if (total_frequency != original_size) {
        std::cerr << "Error: Invalid original size metadata.\n";
        return EXIT_FAILURE_OTHER;
    }

    // REQ-6: Rebuild the tree
    std::shared_ptr<huffman::Node> root;
    try {
        root = huffman::build_tree(freqs);
    } catch (const std::exception& e) {
        std::cerr << "Error rebuilding Huffman tree: " << e.what() << "\n";
        return EXIT_FAILURE_OTHER;
    }
    if (original_size != 0 && !root) {
        std::cerr << "Error: Missing Huffman tree.\n";
        return EXIT_FAILURE_OTHER;
    }

    // [REQ-7 & D-14] Safety: Write to a temporary output file
    std::string temp_path = output_path + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open output file " << temp_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // [NFR #10] Memory Optimization: Read bits stream-by-stream from disk and output directly.
    // Avoids massive vector allocations for payload and decoded bytes.
    if (original_size > 0) {
        if (root->is_leaf) {
            if (root->frequency != original_size || bit_count != original_size) {
                std::cerr << "Error: Invalid single-symbol archive.\n";
                out.close();
                std::filesystem::remove(temp_path);
                return EXIT_FAILURE_OTHER;
            }
            for (std::uint64_t i = 0; i < bit_count; ++i) {
                out.put(static_cast<char>(root->byte));
            }
        } else {
            auto current = root;
            char ch;
            std::uint64_t bits_read = 0;
            std::uint64_t bytes_written = 0;

            while (bits_read < bit_count && in.get(ch)) {
                std::uint8_t byte = static_cast<std::uint8_t>(ch);
                for (int bit_pos = 7; bit_pos >= 0 && bits_read < bit_count; --bit_pos) {
                    bool bit = (byte >> bit_pos) & 1u;
                    current = bit ? current->right : current->left;
                    ++bits_read;

                    if (!current) {
                        std::cerr << "Error: Invalid Huffman bitstream.\n";
                        out.close();
                        std::filesystem::remove(temp_path);
                        return EXIT_FAILURE_OTHER;
                    }

                    if (current->is_leaf) {
                        out.put(static_cast<char>(current->byte));
                        ++bytes_written;
                        current = root;
                        if (bytes_written > original_size) {
                            std::cerr << "Error: Decoded data exceeds original size.\n";
                            out.close();
                            std::filesystem::remove(temp_path);
                            return EXIT_FAILURE_OTHER;
                        }
                    }
                }
            }
            if (current != root || bytes_written != original_size) {
                std::cerr << "Error: Incomplete Huffman code.\n";
                out.close();
                std::filesystem::remove(temp_path);
                return EXIT_FAILURE_OTHER;
            }
        }
    }

    out.close();
    if (!out.good()) {
        std::cerr << "Error: Failed to write decompressed file.\n";
        std::filesystem::remove(temp_path);
        return EXIT_FILE_ERROR;
    }

    std::error_code ec;
    std::filesystem::rename(temp_path, output_path, ec);
    if (ec) {
        std::cerr << "Error renaming temp file: " << ec.message() << "\n";
        std::filesystem::remove(temp_path);
        return EXIT_FILE_ERROR;
    }

    auto written_size = std::filesystem::file_size(output_path, ec);
    if (ec || written_size != original_size) {
        std::cerr << "Error: Lossless matching failed.\n";
        return EXIT_FAILURE_OTHER;
    }

    std::cout << "Successfully decompressed " << input_path << " to " << output_path << "\n";
    std::cout << "Decompressed size: " << original_size << " bytes\n";

    return EXIT_OK;
}