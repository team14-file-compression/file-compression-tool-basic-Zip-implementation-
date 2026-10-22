
#include "huffzip.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using Bytes = std::vector<std::uint8_t>;

static void fail(const std::string& message) {
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

static void write_bytes(const std::string& path, const Bytes& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) fail("Cannot create " + path);

    if (!data.empty()) {
        out.write(reinterpret_cast<const char*>(data.data()),
                  static_cast<std::streamsize>(data.size()));
    }

    if (!out) fail("Cannot write " + path);
}

static Bytes read_bytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) fail("Cannot read " + path);

    return Bytes(std::istreambuf_iterator<char>(in),
                 std::istreambuf_iterator<char>());
}

static void remove_file(const std::string& path) {
    std::remove(path.c_str());
}

static void round_trip(const std::string& name, const Bytes& input) {
    const std::string source = "codec_test_" + name + ".bin";
    const std::string archive = source + ".huff";
    const std::string restored = "codec_restored_" + name + ".bin";

    remove_file(source);
    remove_file(archive);
    remove_file(restored);

    write_bytes(source, input);

    if (compress_file(source, archive) != EXIT_OK)
        fail(name + ": compression failed");

    if (decompress_file(archive, restored) != EXIT_OK)
        fail(name + ": decompression failed");

    if (read_bytes(source) != read_bytes(restored))
        fail(name + ": restored bytes differ from original");

    std::cout << "[PASS] Codec round trip: " << name << '\n';

    remove_file(source);
    remove_file(archive);
    remove_file(restored);
}

void run_codec_tests() {
    round_trip("text", Bytes{
        'h', 'e', 'l', 'l', 'o', ' ', 'H', 'u', 'f', 'f', 'm', 'a', 'n'
    });

    round_trip("empty", Bytes{});

    Bytes binary;
    for (int repeat = 0; repeat < 4; ++repeat) {
        for (int value = 0; value < 256; ++value) {
            binary.push_back(static_cast<std::uint8_t>(value));
        }
    }
    round_trip("binary", binary);

    round_trip("single_byte", Bytes(1000, 65));

    // Corrupt a valid archive's original-size metadata.
    const std::string source = "codec_corrupt_source.bin";
    const std::string archive = "codec_corrupt_source.bin.huff";
    const std::string corrupt = "codec_corrupt.huff";
    const std::string output = "codec_corrupt_output.bin";

    remove_file(source);
    remove_file(archive);
    remove_file(corrupt);
    remove_file(output);

    write_bytes(source, Bytes{'t', 'e', 's', 't'});

    if (compress_file(source, archive) != EXIT_OK)
        fail("Could not create archive for corruption test");

    Bytes archive_data = read_bytes(archive);

    if (archive_data.size() < 8)
        fail("Archive is too short for its size header");

    for (int i = 0; i < 8; ++i)
        archive_data[i] = 0;

    write_bytes(corrupt, archive_data);

    if (decompress_file(corrupt, output) == EXIT_OK)
        fail("Corrupted archive was accepted");

    std::ifstream unexpected_output(output, std::ios::binary);
    if (unexpected_output)
        fail("Corrupted archive left an output file");

    std::cout << "[PASS] Codec rejects corrupted metadata\n";

    remove_file(source);
    remove_file(archive);
    remove_file(corrupt);
    remove_file(output);
}
