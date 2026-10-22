// main.cpp - Command Line Interface (REQ-1, issue #1)
//
// Usage:
//   huffzip -c <input file>  [-o <output file>]   compress   (default output: <input>.huff)
//   huffzip -d <input.huff>  [-o <output file>]   decompress (default output: <input without .huff>.out)
//   huffzip -h                                    show help
#include "huffzip.h"

#include <exception>
#include <filesystem>
#include <iostream>
#include <new>
#include <string>

namespace fs = std::filesystem;

static void print_usage() {
    std::cout <<
        "File Compression Tool (Huffman coding) - Team 14\n"
        "Usage:\n"
        "  huffzip -c <input file> [-o <output file>]   Compress a file\n"
        "  huffzip -d <input.huff> [-o <output file>]   Decompress a file\n"
        "  huffzip -h                                   Show this help\n";
}

// True if both paths name the same file (also catches "./a.txt" vs "a.txt").
static bool same_file(const std::string& a, const std::string& b) {
    if (a == b) return true;
    std::error_code ec;
    return fs::exists(b, ec) && fs::equivalent(a, b, ec);
}

static bool ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() &&
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "-h") {
        print_usage();
        return EXIT_OK;
    }
    if (argc != 3 && argc != 5) {
        std::cerr << "Error: wrong number of arguments\n";
        print_usage();
        return EXIT_USAGE;
    }

    const std::string mode = argv[1];
    const std::string input = argv[2];
    std::string output;

    if (mode != "-c" && mode != "-d") {
        std::cerr << "Error: unknown option '" << mode << "'\n";
        print_usage();
        return EXIT_USAGE;
    }
    if (argc == 5) {
        if (std::string(argv[3]) != "-o") {
            std::cerr << "Error: expected -o before the output file name\n";
            print_usage();
            return EXIT_USAGE;
        }
        output = argv[4];
    }

    std::string error;
    if (!validate_input_file(input, error)) {
        std::cerr << "Error: " << error << "\n";
        return EXIT_FILE_ERROR;
    }

    if (output.empty()) {
        if (mode == "-c") {
            output = input + ".huff";
        } else {
            output = ends_with(input, ".huff") ? input.substr(0, input.size() - 5) + ".out"
                                               : input + ".out";
        }
    }

    // SRS 6.2: never destroy user data. Writing the output over the input
    // would delete the original file.
    if (same_file(input, output)) {
        std::cerr << "Error: output file must be different from the input file\n";
        return EXIT_USAGE;
    }

    // SRS 6.2: terminate gracefully with a helpful message instead of crashing.
    try {
        return mode == "-c" ? compress_file(input, output)
                            : decompress_file(input, output);
    } catch (const std::bad_alloc&) {
        std::cerr << "Error: not enough memory to process '" << input << "'\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
    return EXIT_FAILURE_OTHER;
}
