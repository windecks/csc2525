#include "lz77.h"

#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <vector>
#include "bit_reader.h"
#include "bit_writer.h"
#include "constants.h"
#include "hash_chain.h"

namespace {
    uint32_t bits_needed(const uint64_t n) {
        if (n == 0)
            return 1;
        return 64 - __builtin_clzll(n);
    }
} // namespace

void LZ77::compress(const std::string &input_file, const std::string &output_file) {
    std::ifstream infile(input_file, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening input file: " << input_file << std::endl;
        return;
    }
    std::ofstream outfile(output_file, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    const size_t search_buffer_size = config.search_buffer_size;
    const size_t lookahead_buffer_size = config.lookahead_buffer_size;

    size_t total_size = search_buffer_size + lookahead_buffer_size;
    std::vector<char> buffer(total_size);
    infile.read(buffer.data(), total_size);
    size_t bytes_in_buffer = infile.gcount();

    size_t bits_for_offset = bits_needed(search_buffer_size);
    size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitWriter bit_writer(outfile);

    HashChain hash_chain(search_buffer_size, config.max_chain_attempts);
    size_t buffer_cursor = 0;
    size_t absolute_pos = 0;
    size_t buffer_start_abs = 0;

    double ema_mean = csc2525::MIN_MATCH_LENGTH;
    double ema_mad = 0.0;
    size_t match_count = 1;

    while (buffer_cursor < bytes_in_buffer) {
        const size_t max_lookahead = std::min(lookahead_buffer_size, bytes_in_buffer - buffer_cursor - 1);
        const size_t search_start_abs = (absolute_pos < search_buffer_size) ? 0 : absolute_pos - search_buffer_size;

        size_t offset = 0, length = 0;
        size_t good_enough_length = std::max<size_t>(csc2525::MIN_MATCH_LENGTH,
                                                     static_cast<size_t>(ema_mean * config.good_enough_multiplier));

        // only use hash lookup if we have at enough bytes to look at
        if (buffer_cursor + csc2525::MIN_MATCH_LENGTH <= bytes_in_buffer &&
            max_lookahead >= csc2525::MIN_MATCH_LENGTH) {
            auto [off, len] = hash_chain.find_good_enough_match(buffer.data(), buffer_start_abs, absolute_pos,
                                                                search_start_abs, max_lookahead, good_enough_length);
            offset = off;
            length = len;

            if (length >= csc2525::MIN_MATCH_LENGTH) {
                const double diff = static_cast<double>(length) - ema_mean;
                if (match_count++ < 20 || std::abs(diff) <= config.outlier_k * ema_mad) {
                    ema_mean += config.decay_rate * diff;
                    ema_mad += config.decay_rate * (std::abs(diff) - ema_mad);
                }
            }
        }

        // write compressed tuple
        const char next_char = buffer[buffer_cursor + length];
        bit_writer.write_bits(offset, bits_for_offset);
        bit_writer.write_bits(length, bits_for_length);
        bit_writer.write_bits(static_cast<unsigned char>(next_char), 8);

        // Insert positions into hash chain
        for (size_t i = 0; i <= length && buffer_cursor + i + csc2525::MIN_MATCH_LENGTH <= bytes_in_buffer; ++i) {
            hash_chain.insert(buffer.data() + buffer_cursor + i, absolute_pos + i);
        }

        buffer_cursor += (length + 1);
        absolute_pos += (length + 1);

        // if we are near the end of the buffer, shift and read more data
        if (buffer_cursor > search_buffer_size) {
            const size_t shift_amount = buffer_cursor - search_buffer_size;
            const size_t remaining_bytes = bytes_in_buffer - shift_amount;
            std::memmove(buffer.data(), buffer.data() + shift_amount, remaining_bytes);
            infile.read(buffer.data() + remaining_bytes, shift_amount);
            size_t new_bytes = infile.gcount();
            bytes_in_buffer = remaining_bytes + new_bytes;
            buffer_cursor -= shift_amount;
            buffer_start_abs += shift_amount;
        }

        if (bytes_in_buffer == 0) {
            break;
        }
    }
}

void LZ77::decompress(const std::string &input_file, const std::string &output_file) {
    std::ifstream infile(input_file, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening input file: " << input_file << std::endl;
        return;
    }
    std::ofstream outfile(output_file, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    const size_t search_buffer_size = config.search_buffer_size;
    const size_t lookahead_buffer_size = config.lookahead_buffer_size;

    std::vector<char> window(search_buffer_size);
    size_t write_pos = 0;

    const size_t bits_for_offset = bits_needed(search_buffer_size);
    const size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitReader bit_reader(infile);

    uint32_t offset_val, length_val, next_char_val;
    while (bit_reader.read_bits(offset_val, bits_for_offset)) {
        if (!bit_reader.read_bits(length_val, bits_for_length))
            break;
        if (!bit_reader.read_bits(next_char_val, 8))
            break;

        const size_t offset = offset_val;
        const size_t length = length_val;
        char next_char = static_cast<char>(next_char_val);

        if (offset > 0) {
            for (size_t i = 0; i < length; ++i) {
                size_t read_idx = (write_pos - offset) % search_buffer_size;
                char c = window[read_idx];
                outfile.put(c);
                window[write_pos % search_buffer_size] = c;
                write_pos++;
            }
        }
        window[write_pos % search_buffer_size] = next_char;
        write_pos++;
        outfile.put(next_char);
    }
}
