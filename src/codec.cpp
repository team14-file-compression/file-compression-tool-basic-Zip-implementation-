// codec.cpp - compression and decompression (REQ-3 to REQ-7)
#include "huffzip.h"
#include "huffman.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <limits>
#include <memory>
#include <cstdint>
#include <exception>
#include <array>

int compress_file(const std::string& input_path, const std::string& output_path) {
    std::string error;
    std::vector<uint8_t> input_bytes;

    // REQ-1: Read input file.
    if (!read_file_bytes(input_path, input_bytes, error)) {
        std::cerr << "Error reading input: " << error << "\n";
        return EXIT_FILE_ERROR;
    }

    // REQ-2: Calculate frequencies.
    FreqTable freqs = count_frequencies(input_bytes);

    // REQ-3: Build tree, generate codes
    auto root = huffman::build_tree(freqs);
    auto codes = huffman::generate_codes(root);

    // [NFR #10] Memory Optimization: Calculate exact bit count without building an enormous vector
    uint64_t bit_count = 0;
    for (size_t i = 0; i < 256; ++i) {
        bit_count += freqs[i] * codes[i].length();
    }

    // [REQ-7 & D-14] Safety: Write to a temporary file first
    std::string temp_path = output_path + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open output file " << temp_path << "\n";
        return EXIT_FILE_ERROR;
    }

    // REQ-4: Write binary .huff archive header
    const std::uint64_t original_size = input_bytes.size();
    out.write(reinterpret_cast<const char*>(&original_size), sizeof(original_size));
    out.write(reinterpret_cast<const char*>(freqs.data()), sizeof(std::uint64_t) * freqs.size());
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

    // [REQ-7 & D-14] Success: safely rename temp file
    std::error_code ec;
    std::filesystem::rename(temp_path, output_path, ec);
    if (ec) {
        std::cerr << "Error renaming temp file: " << ec.message() << "\n";
        std::filesystem::remove(temp_path);
        return EXIT_FILE_ERROR;
    }

    // Calculate metrics
    const std::uint64_t compressed_size = 8 + (sizeof(std::uint64_t) * freqs.size()) + 8 + (bit_count / 8 + (bit_count % 8 != 0 ? 1 : 0));
    double ratio = 100.0;
    if (original_size > 0) {
        ratio = (static_cast<double>(compressed_size) / static_cast<double>(original_size)) * 100.0;
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
    if (!in) {
        std::cerr << "Error: Truncated archive header.\n";
        return EXIT_FAILURE_OTHER;
    }

    FreqTable freqs{};
    in.read(reinterpret_cast<char*>(freqs.data()), sizeof(std::uint64_t) * freqs.size());
    if (!in) {
        std::cerr << "Error: Incomplete frequency table.\n";
        return EXIT_FAILURE_OTHER;
    }

    std::uint64_t bit_count = 0;
    in.read(reinterpret_cast<char*>(&bit_count), sizeof(bit_count));
    if (!in) {
        std::cerr << "Error: Truncated archive header.\n";
        return EXIT_FAILURE_OTHER;
    }

    // Validate metadata
    std::uint64_t total_frequency = 0;
    for (std::uint64_t frequency : freqs) {
        total_frequency += frequency;
    }
    if (total_frequency != original_size) {
        std::cerr << "Error: Invalid original size metadata.\n";
        return EXIT_FAILURE_OTHER;
    }

    if (original_size == 0 && bit_count != 0) {
        std::cerr << "Error: Invalid empty-file bit count.\n";
        return EXIT_FAILURE_OTHER;
    }
    if (original_size != 0 && bit_count == 0) {
        std::cerr << "Error: Missing encoded bits.\n";
        return EXIT_FAILURE_OTHER;
    }

    // REQ-6: Rebuild the deterministic Huffman tree
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

    // [REQ-7 & D-14] Safety: Write to a temp file
    std::string temp_path = output_path + ".tmp";
    std::ofstream out(temp_path, std::ios::binary);
    if (!out) {
        std::cerr << "Error: Could not open output file " << temp_path << "\n";
        return EXIT_FILE_ERROR;
    }

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
            // Rachit's Lookup Table Optimization
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
                    if (!node) break;
                    if (node->is_leaf) {
                        table[prefix].byte = node->byte;
                        table[prefix].bits_used = depth;
                        table[prefix].valid = true;
                        break;
                    }
                }
            }

            // [NFR #10] Stream Integration: Apply lookup table directly to file streams
            auto current = root;
            std::uint64_t i = 0;
            std::uint64_t bytes_written = 0;
            char ch;
            std::uint8_t current_byte = 0;

            while (i < bit_count) {
                const std::uint64_t remaining = bit_count - i;

                if (i % 8 == 0) {
                    if (!in.get(ch)) {
                        std::cerr << "Error: Truncated bitstream on disk.\n";
                        out.close();
                        std::filesystem::remove(temp_path);
                        return EXIT_FILE_ERROR;
                    }
                    current_byte = static_cast<std::uint8_t>(ch);
                }

                // Fast path: use table at byte boundaries
                if (current == root && (i % 8 == 0) && remaining >= LOOKUP_BITS) {
                    const DecodeEntry& entry = table[current_byte];
                    if (entry.valid) {
                        out.put(static_cast<char>(entry.byte));
                        ++bytes_written;
                        i += entry.bits_used;

                        if (bytes_written > original_size) {
                            std::cerr << "Error: Decoded data exceeds original size.\n";
                            out.close();
                            std::filesystem::remove(temp_path);
                            return EXIT_FAILURE_OTHER;
                        }
                        continue;
                    }
                }

                // Safe fallback: traverse bit by bit
                const unsigned shift = 7u - static_cast<unsigned>(i % 8);
                const bool bit = ((current_byte >> shift) & 1u) != 0;

                current = bit ? current->right : current->left;
                if (!current) {
                    std::cerr << "Error: Invalid Huffman bitstream.\n";
                    out.close();
                    std::filesystem::remove(temp_path);
                    return EXIT_FAILURE_OTHER;
                }
                
                ++i;

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