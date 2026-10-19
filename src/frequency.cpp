// frequency.cpp - REQ-2: calculate character (byte) frequencies (issue #2)
#include "huffzip.h"

FreqTable count_frequencies(const std::vector<std::uint8_t>& data) {
    FreqTable freq{};  // all 256 counts start at 0
    for (std::uint8_t byte : data) {
        ++freq[byte];
    }
    return freq;
}
