#pragma once

#include <cstddef>
#include <string>
#include "compressor.h"

struct LZ77Config {
    size_t search_buffer_size = 32768;
    size_t lookahead_buffer_size = 256;
    size_t max_chain_attempts = 20;
    double decay_rate = 0.05;
    double good_enough_multiplier = 1.5;
    double outlier_k = 2.0;
    bool lazy_parsing = true;
};

class LZ77 : public Compressor {
private:
    LZ77Config config;
public:
    explicit LZ77(LZ77Config config) 
        : config(config) {}
    void compress(const std::string &input_file, const std::string &output_file) override;
    void decompress(const std::string &input_file, const std::string &output_file) override;
};
