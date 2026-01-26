#include "lz77.h"

#include <iostream>

void print_compressed(const std::vector<std::tuple<int, int, char>> &compressed) {
    for (const auto &[offset, length, literal]: compressed) {
        std::cout << "(" << offset << ", " << length << ", '" << literal << "') ";
    }
    std::cout << std::endl;
}

int main() {
    std::string INPUT = "HELLOHELLJELLO";
    auto compressed = LZ77::compress(5, INPUT.c_str());
    print_compressed(compressed);
    std::string decompressed = LZ77::decompress(compressed);
    std::cout << "Decompressed: " << decompressed << std::endl;
    return 0;
}
