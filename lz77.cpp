#include "lz77.h"

#include <cstring>
#include <deque>
#include <fstream>
#include <vector>
#include "bit_reader.h"
#include "bit_writer.h"
#include "hash_chain.h"

namespace {
    uint32_t bits_needed(const uint64_t n) {
        if (n <= 1)
            return n;
        return 64 - __builtin_clzll(n - 1);
    }
} // namespace

void LZ77::compress(const size_t &search_buffer_size, const size_t &lookahead_buffer_size, std::ifstream &infile,
                    std::ofstream &outfile) {
    size_t total_size = search_buffer_size + lookahead_buffer_size;
    std::vector<char> buffer(total_size);
    infile.read(buffer.data(), total_size);
    size_t bytes_in_buffer = infile.gcount();

    size_t bits_for_offset = bits_needed(search_buffer_size);
    size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitWriter bit_writer(outfile);

    HashChain hash_chain(search_buffer_size);
    size_t buffer_cursor = 0;
    size_t absolute_pos = 0;
    size_t buffer_start_abs = 0;

    while (buffer_cursor < bytes_in_buffer) {
        const size_t max_lookahead = std::min(lookahead_buffer_size, bytes_in_buffer - buffer_cursor - 1);
        const size_t search_start_abs = (absolute_pos < search_buffer_size) ? 0 : absolute_pos - search_buffer_size;

        size_t offset = 0, length = 0;

        // only use hash lookup if we have at enough bytes to look at
        if (buffer_cursor + csc2525::MIN_MATCH_LENGTH <= bytes_in_buffer &&
            max_lookahead >= csc2525::MIN_MATCH_LENGTH) {
            auto [off, len] = hash_chain.find_longest_match(buffer.data(), buffer_start_abs, absolute_pos,
                                                            search_start_abs, max_lookahead);
            offset = off;
            length = len;
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

            // cleanup
            hash_chain.evict_old(absolute_pos);
        }

        if (bytes_in_buffer == 0) {
            break;
        }
    }
}

void LZ77::decompress(const size_t &search_buffer_size, const size_t &lookahead_buffer_size, std::ifstream &infile,
                      std::ofstream &outfile) {
    std::vector<char> window;
    window.reserve(search_buffer_size);

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
            size_t start_index = window.size() - offset;
            for (size_t i = 0; i < length; ++i) {
                char c = window[start_index + i];
                outfile.write(&c, sizeof(char));
                window.push_back(c);
            }
        }
        window.push_back(next_char);
        outfile.write(&next_char, sizeof(char));

        // Maintain the search buffer size
        if (window.size() > search_buffer_size) {
            window.erase(window.begin(), window.begin() + (window.size() - search_buffer_size));
        }
    }
}
