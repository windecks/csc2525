//
// Created by zhujose1 on 2026-02-18.
//

#pragma once

#include <stdexcept>
#include <stdint.h>
#include <vector>
#include "constants.h"

struct BitReader {
    std::ifstream &file;
    std::vector<uint8_t> buffer;
    uint64_t bit_accumulator;
    size_t current_pos;
    size_t bytes_in_buffer;
    int bit_count;

    BitReader(std::ifstream &f) :
        file(f), buffer(csc2525::BIT_WRITER_BUFFER_SIZE), bit_accumulator(0),
        current_pos(0), bytes_in_buffer(0), bit_count(0) {
        refill_buffer();
    }

    bool read_bits(uint32_t &out, const int& nbits) {
        if (nbits <= 0 || nbits > 32) {
            throw std::invalid_argument("nbits must be between 1 and 32");
        }
        while (bit_count < nbits) {
            if (current_pos >= bytes_in_buffer) {
                refill_buffer();
                if (bytes_in_buffer == 0) {
                    // EOF and not enough bits
                    return false;
                }
            }
            bit_accumulator |= static_cast<uint64_t>(buffer[current_pos++]) << bit_count;
            bit_count += 8;
        }

        out = static_cast<uint32_t>(bit_accumulator & ((1ULL << nbits) - 1));
        bit_accumulator >>= nbits;
        bit_count -= nbits;
        return true;
    }

private:
    void refill_buffer() {
        file.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
        bytes_in_buffer = file.gcount();
        current_pos = 0;
    }
};
