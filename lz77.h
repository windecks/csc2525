#include <cstddef>
#include <string>

#pragma once

static class LZ77 {
public:
    static void compress(size_t search_buffer_size, size_t lookahead_buffer_size, std::ifstream &infile,
                         std::ofstream &outfile);
    static void decompress(size_t search_buffer_size, size_t lookahead_buffer_size, std::ifstream &infile,
                           std::ofstream &outfile);
} LZ77;

