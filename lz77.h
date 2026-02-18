#pragma once

#include <cstddef>
#include <string>

static class LZ77 {
public:
    static void compress(const size_t& search_buffer_size, const size_t& lookahead_buffer_size, std::ifstream &infile,
                         std::ofstream &outfile);
    static void decompress(const size_t& search_buffer_size, const size_t& lookahead_buffer_size, std::ifstream &infile,
                           std::ofstream &outfile);
} LZ77;
