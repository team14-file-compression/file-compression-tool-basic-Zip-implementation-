
#include "huffzip.h"
#include "huffman.hpp"

#include <iostream>

// Implemented in tests/test_huffman.cpp (Pavan)
void run_huffman_tests();

// Implemented in tests/test_input.cpp (Pranavi)
void run_input_tests();

// Implemented in tests/test_codec.cpp (Rachit)
void run_codec_tests();

int main() {
    std::cout << "Running project tests...\n";

    run_huffman_tests();
    run_input_tests();
    run_codec_tests();

    std::cout << "All tests passed\n";
    return 0;
}
