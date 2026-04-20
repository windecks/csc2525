#pragma once

#include <cstddef>
#include <string>
#include "compressor.h"

class LZ77 : public Compressor {
private:
    size_t search_buffer_size;
    size_t lookahead_buffer_size;
public:
    LZ77(size_t search_size, size_t lookahead_size) 
        : search_buffer_size(search_size), lookahead_buffer_size(lookahead_size) {}
    void compress(const std::string &input_file, const std::string &output_file) override;
    void decompress(const std::string &input_file, const std::string &output_file) override;
};
