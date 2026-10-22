// codec.cpp - compression and decompression (REQ-3 to REQ-7)
#include "huffzip.h"
#include "huffman.hpp"

#include <iostream>
#include <fstream>
#include <vector>

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

int decompress_file(const std::string& input_path, const std::string& output_path) {
    (void)input_path;
    (void)output_path;
    std::cerr << "Error: decompression is not implemented yet\n";
    return EXIT_FAILURE_OTHER;
}