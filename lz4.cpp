#include "lz4.h"
#include "mapped_file.h"
#include <array>
#include <fstream>
#include <iostream>
#include <unistd.h>
#include <vector>

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
        constexpr uint8_t max_len = std::numeric_limits<uint8_t>::max();
        while (len >= max_len) {
            outfile.put(static_cast<char>(max_len));
            len -= max_len;
        }
        outfile.put(static_cast<char>(len));
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
    const MappedFile file(input_file);
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
            while (match_pos + match_len < data_len &&
                   data[match_pos + match_len] == data[prev_match_pos + match_len]) {
                match_len++;
            }

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
}
