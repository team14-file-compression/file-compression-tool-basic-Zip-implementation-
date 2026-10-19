// file_input.cpp - REQ-1: accept and validate the input file path (issue #1)
#include "huffzip.h"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

bool validate_input_file(const std::string& path, std::string& error) {
    if (path.empty()) {
        error = "no input file given";
        return false;
    }
    std::error_code ec;
    if (!fs::exists(path, ec)) {
        error = "input file '" + path + "' not found";
        return false;
    }
    if (!fs::is_regular_file(path, ec)) {
        error = "'" + path + "' is not a regular file";
        return false;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot open '" + path + "' for reading (check permissions)";
        return false;
    }
    return true;
}

bool read_file_bytes(const std::string& path, std::vector<std::uint8_t>& out,
                     std::string& error) {
    std::ifstream in(path, std::ios::binary);  // binary: no \r\n changes on Windows
    if (!in) {
        error = "cannot open '" + path + "' for reading";
        return false;
    }
    in.seekg(0, std::ios::end);
    std::streamoff size = in.tellg();
    in.seekg(0, std::ios::beg);
    if (size < 0) {
        error = "cannot determine size of '" + path + "'";
        return false;
    }
    out.resize(static_cast<std::size_t>(size));
    if (size > 0 && !in.read(reinterpret_cast<char*>(out.data()), size)) {
        error = "error while reading '" + path + "'";
        return false;
    }
    return true;
}
