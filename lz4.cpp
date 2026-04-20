#include "lz4.h"
#include <array>
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <unistd.h>
#include <vector>
#include "mapped_file.h"
#include "rotating_window.h"

namespace {
    constexpr size_t HASH_SIZE = 1 << 16;
    constexpr size_t MAX_OFFSET = std::numeric_limits<uint16_t>::max();

    // Basic hash function for 4 bytes (LZ4 minimum match) (Fibonacci hashing)
    uint32_t hash4(const char *p) {
        uint32_t x;
        std::memcpy(&x, p, 4);
        return ((x * 2654435761U) >> 16) % HASH_SIZE;
    }

    void write_len(size_t len, std::ofstream &outfile) {
        do {
            outfile.put(static_cast<char>((len & ((1 << 7) - 1)) | (((len >> 7) > 0) << 7)));
        } while (len >>= 7);
    }

    uint32_t read_len(const char *&loc) {
        uint32_t len = 0;
        uint8_t byte, shift = 0;
        do {
            len |= ((byte = *(loc++)) & ((1 << 7) - 1)) << shift;
            shift += 7;
        } while (byte & (1 << 7));
        return len;
    }

    void write_literals(const uint32_t literal_pos, const uint32_t literal_len, const uint32_t match_len,
                        const char *data, std::ofstream &outfile) {
        const uint8_t token = (std::min(literal_len, 15u) << 4) | std::min(match_len - 4, 15u);
        outfile.put(static_cast<const char>(token));

        if (literal_len >= 15)
            write_len(literal_len - 15, outfile);

        outfile.write(&data[literal_pos], literal_len);
    }

    void write_match(const uint32_t match_pos, const uint32_t prev_match_pos, const uint32_t match_len,
                     std::ofstream &outfile) {
        // offset is encoded as a 2-byte little endian integer
        const auto offset = static_cast<uint16_t>(match_pos - prev_match_pos);
        outfile.put(static_cast<char>(offset & 0xFF));
        outfile.put(static_cast<char>((offset >> 8) & 0xFF));

        // we subtract 4 since 4 is the minimum allowed length of the match
        if (match_len - 4 >= 15)
            write_len(match_len - (4 + 15), outfile);
    }
} // namespace

void LZ4::compress(const std::string &input_file, const std::string &output_file) {
    const MappedFile<mode::read> file(input_file);
    if (!file.is_valid())
        return;

    const size_t data_len = file.size();
    if (data_len == 0)
        return;

    const char *data = file.data();

    std::ofstream outfile(output_file, std::ios::binary);
    if (!outfile) {
        std::cerr << "Error opening output file: " << output_file << std::endl;
        return;
    }

    write_len(data_len, outfile);
    std::vector<uint32_t> hash_table(HASH_SIZE, ~0);

    size_t literal_pos = 0;
    size_t match_pos = 0;

    while (match_pos < data_len) {
        /**
         * https://android.googlesource.com/platform/external/lz4/+/HEAD/doc/lz4_Block_format.md#end-of-block-conditions
         * The last match must start at least 12 bytes before the end of block. The last match is part of the
         * penultimate sequence. It is followed by the last sequence, which contains only literals.
         */
        if (match_pos + 12 > data_len)
            break;

        const uint32_t hash = hash4(&data[match_pos]);
        const uint32_t prev_match_pos = hash_table[hash];
        hash_table[hash] = static_cast<uint32_t>(match_pos);

        if (prev_match_pos != ~0 // match position is valid
            && (match_pos - prev_match_pos) < MAX_OFFSET // offset is valid
            && std::memcmp(&data[match_pos], &data[prev_match_pos], 4) == 0) {

            size_t match_len = 4;
#ifdef BTYTE_BY_BYTE
            while (match_pos + match_len + 5 < data_len &&
                   data[match_pos + match_len] == data[prev_match_pos + match_len]) {
                match_len++;
            }
#else
            while (match_pos + match_len + 8 <= data_len) {
                uint64_t curr, prev;
                memcpy(&curr, data + match_pos + match_len, 8);
                memcpy(&prev, data + prev_match_pos + match_len, 8);
                if (curr == prev) {
                    match_len += 8;
                } else {
                    int difference_in;
                    if constexpr (std::endian::native == std::endian::little) {
                        difference_in = __builtin_ctzll(curr ^ prev) >> 3;
                    } else {
                        difference_in = __builtin_clzll(curr ^ prev) >> 3;
                    }
                    match_len += difference_in;
                    if (!difference_in)
                        break;
                }
            }
#endif

            write_literals(literal_pos, match_pos - literal_pos, match_len, data, outfile);
            write_match(match_pos, prev_match_pos, match_len, outfile);

            literal_pos = (match_pos += match_len);
        } else {
            match_pos++;
        }
    }

    // we are passing 4 as match_len since we subtract 4 when generating the token
    write_literals(literal_pos, data_len - literal_pos, 4, data, outfile);
}

void LZ4::decompress(const std::string &input_file, const std::string &output_file) {
    const MappedFile<mode::read> input(input_file);
    if (!input.is_valid())
        return;

    const size_t input_len = input.size();
    if (input_len == 0)
        return;

    const char *input_data = input.data(), *input_pos = input_data;

    MappedFile<mode::write> output(output_file, read_len(input_pos));
    if (!output.is_valid())
        return;

    char *output_data = output.data(), *output_pos = output_data;

    while (input_pos < input_data + input_len) {
        const auto token = static_cast<uint8_t>(*(input_pos++));
        uint32_t literal_len = token >> 4;

        if (literal_len == 15)
            literal_len += read_len(input_pos);

        memcpy(output_pos, input_pos, literal_len);
        output_pos += literal_len;
        input_pos += literal_len;

        if (input_pos >= input_data + input_len)
            break;


        const uint16_t offset =
                static_cast<uint8_t>(input_pos[0]) | (static_cast<uint16_t>(static_cast<uint8_t>(input_pos[1])) << 8);
        input_pos += 2;

        uint32_t match_len = (token & 0x0F);
        if (match_len == 15)
            match_len += read_len(input_pos);
        match_len += 4; // match length is always at least 4

        const auto match_from = output_pos - offset;
        while (match_len) {
            const auto match = std::min(match_len, static_cast<uint32_t>(output_pos - match_from));
            memcpy(output_pos, match_from, match);
            output_pos += match;
            match_len -= match;
        }
    }
}
