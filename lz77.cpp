#include "lz77.h"

#include <cstring>
#include <fstream>
#include <utility>
#include <vector>
#include "bit_reader.h"
#include "bit_writer.h"

namespace {
    // Finds the longest match of the lookahead buffer in the search buffer, up to max_length
    std::pair<size_t, size_t> findLongestMatch(const char *start, const char *current, size_t max_length) {
        size_t best_offset = 0;
        size_t best_length = 0;

        for (const char *p = start; p < current; ++p) {
            size_t length = 0;
            while (length < max_length && p[length] == current[length]) {
                length++;
            }

            if (length > best_length) {
                best_length = length;
                best_offset = current - p;
            }
        }
        return {best_offset, best_length};
    }

    uint32_t bits_needed(const uint64_t n) {
        if (n <= 1)
            return n;
        return 64 - __builtin_clzll(n - 1);
    }
} // namespace

void LZ77::compress(size_t search_buffer_size, size_t lookahead_buffer_size, std::ifstream &infile,
                    std::ofstream &outfile) {
    size_t total_size = search_buffer_size + lookahead_buffer_size;
    std::vector<char> buffer(total_size);
    infile.read(buffer.data(), total_size);
    size_t bytes_in_buffer = infile.gcount();

    size_t bits_for_offset = bits_needed(search_buffer_size);
    size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitWriter bit_writer(outfile);

    size_t buffer_cursor = 0;
    while (buffer_cursor < bytes_in_buffer) {
        const size_t max_lookahead = std::min(lookahead_buffer_size, bytes_in_buffer - buffer_cursor - 1);
        const size_t search_offset = (buffer_cursor < search_buffer_size) ? 0 : buffer_cursor - search_buffer_size;
        const char *search_start = buffer.data() + search_offset;
        const char *current = buffer.data() + buffer_cursor;
        auto [offset, length] = findLongestMatch(search_start, current, max_lookahead);

        // Write the (offset, length, next char) to the output file
        char next_char = buffer[buffer_cursor + length];
        bit_writer.write_bits(offset, bits_for_offset);
        bit_writer.write_bits(length, bits_for_length);
        bit_writer.write_bits(static_cast<unsigned char>(next_char), 8);
        buffer_cursor += (length + 1);

        // If we are near the end of the buffer, read more data
        if (buffer_cursor > search_buffer_size) {
            const size_t shift_amount = buffer_cursor - search_buffer_size;
            const size_t remaining_bytes = bytes_in_buffer - shift_amount;
            std::memmove(buffer.data(), buffer.data() + shift_amount, remaining_bytes);
            infile.read(buffer.data() + remaining_bytes, shift_amount);
            bytes_in_buffer = remaining_bytes + infile.gcount();
            buffer_cursor -= shift_amount;
        }
        if (bytes_in_buffer == 0) {
            break;
        }
    }
}

void LZ77::decompress(size_t search_buffer_size, size_t lookahead_buffer_size, std::ifstream &infile,
                      std::ofstream &outfile) {
    std::vector<char> window;
    window.reserve(search_buffer_size);

    size_t bits_for_offset = bits_needed(search_buffer_size);
    size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitReader bit_reader(infile);

    uint32_t offset_val, length_val, next_char_val;
    while (bit_reader.read_bits(offset_val, bits_for_offset)) {
        if (!bit_reader.read_bits(length_val, bits_for_length))
            break;
        if (!bit_reader.read_bits(next_char_val, 8))
            break;

        size_t offset = offset_val;
        size_t length = length_val;
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
