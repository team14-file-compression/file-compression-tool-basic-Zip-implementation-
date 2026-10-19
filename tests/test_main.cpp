// test_main.cpp - shared test runner. Build and run with:  make test
// Each team member keeps their tests in their own file (tests/test_<name>.cpp)
// with one run_..._tests() function, declared and called below.
#include "huffzip.h"
#include "huffman.hpp"

#include <iostream>

// Implemented in tests/test_huffman.cpp (Pavan)
void run_huffman_tests();
// Implemented in tests/test_input.cpp (Pranavi)
void run_input_tests();

int main() {
    std::cout << "Running project tests...\n";

    run_huffman_tests();
    run_input_tests();

    std::cout << "All tests passed\n";
    return 0;
}
