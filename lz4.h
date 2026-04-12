#pragma once

#include <cstddef>
#include <string>

class LZ4 {
public:
    static void compress(const std::string& input_file, const std::string& output_file);
    static void decompress(const std::string& input_file, const std::string& output_file);
};
