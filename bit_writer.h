//
// Created by zhujose1 on 2026-02-18.
//

#pragma once

#include <stdexcept>
#include <stdint.h>
#include <vector>
#include "constants.h"

struct BitWriter {
    std::ofstream &file;
    std::vector<uint8_t> buffer;
    uint64_t bit_accumulator;
    size_t current_pos;
    int bit_count;
    BitWriter(std::ofstream &f) :
        file(f), buffer(csc2525::BIT_WRITER_BUFFER_SIZE), bit_count(0), bit_accumulator(0), current_pos(0) {}

    ~BitWriter() { flush_bits(); }
    void write_bits(uint32_t bits, int nbits) {
        if (nbits <= 0 || nbits > 32) {
            throw std::invalid_argument("nbits must be between 1 and 32");
        }
        bit_accumulator |= static_cast<uint64_t>(bits & ((1ULL << nbits) - 1)) << bit_count;
        bit_count += nbits;
        // write bits in accumulator into the buf
        while (bit_count >= 8) {
            buffer[current_pos++] = static_cast<uint8_t>(bit_accumulator & 0xFF);
            bit_accumulator >>= 8;
            bit_count -= 8;
            if (current_pos >= buffer.size()) {
                write_buffer_to_file();
            }
        }
    }

    void flush_bits() {
        if (bit_count > 0) {
            buffer[current_pos++] = static_cast<uint8_t>(bit_accumulator & 0xFF);
            bit_accumulator = 0;
            bit_count = 0;
        }
        if (current_pos > 0) {
            write_buffer_to_file();
        }
    }

private:
    void write_buffer_to_file() {
        if (current_pos == 0)
            return;

        file.write(reinterpret_cast<const char *>(buffer.data()), current_pos);
        current_pos = 0;
    }
};
