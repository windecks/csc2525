#pragma once

#include <cstddef>
#include <string>
#include "compressor.h"

class LZ4 : public Compressor {
public:
    void compress(const std::string &input_file, const std::string &output_file) override;
    void decompress(const std::string &input_file, const std::string &output_file) override;
};
