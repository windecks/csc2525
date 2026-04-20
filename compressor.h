#pragma once
#include <string>

class Compressor {
public:
    virtual ~Compressor() = default;
    virtual void compress(const std::string& input_file, const std::string& output_file) = 0;
    virtual void decompress(const std::string& input_file, const std::string& output_file) = 0;
};
