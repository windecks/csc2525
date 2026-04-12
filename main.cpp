#include "lz77.h"

#include <cstring>
#include <fstream>
#include <iostream>

#include "bit_reader.h"
#include "constants.h"
#include "lz4.h"

namespace {
    uint32_t bits_needed(const uint64_t n) {
        if (n <= 1)
            return n;
        return 64 - __builtin_clzll(n - 1);
    }
} // namespace

void print_compressed(const char *filename, size_t search_buffer_size, size_t lookahead_buffer_size) {
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        std::cerr << "Error opening compressed file: " << filename << std::endl;
        return;
    }

    size_t bits_for_offset = bits_needed(search_buffer_size);
    size_t bits_for_length = bits_needed(lookahead_buffer_size);
    BitReader bit_reader(infile);

    uint32_t offset, length, next_char;
    std::cout << "Compressed data (offset, length, next_char):" << std::endl;
    while (bit_reader.read_bits(offset, bits_for_offset)) {
        if (!bit_reader.read_bits(length, bits_for_length))
            break;
        if (!bit_reader.read_bits(next_char, 8))
            break;
        std::cout << "(" << offset << ", " << length << ", '" << static_cast<char>(next_char) << "')" << std::endl;
    }
}

void open_infile_or_exit(std::ifstream &file, const char *filename) {
    file.open(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(1);
    }
}

void open_outfile_or_exit(std::ofstream &file, const char *filename) {
    file.open(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error opening file: " << filename << std::endl;
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " [lz77/lz4] infile compressed outfile" << std::endl;
        return 1;
    }

    if (memcmp(argv[1], "lz4", strlen("lz4")) == 0) {
        LZ4::compress(argv[2], argv[3]);
        LZ4::decompress(argv[3], argv[4]);
    } else if (memcmp(argv[1], "lz77", strlen("lz77")) == 0) {
        std::ifstream infile;
        open_infile_or_exit(infile, argv[2]);
        std::ofstream outfile;
        open_outfile_or_exit(outfile, argv[3]);

        LZ77::compress(csc2525::SEARCH_BUFFER_SIZE, csc2525::LOOKAHEAD_BUFFER_SIZE, infile, outfile);
        infile.close();
        outfile.close();

        open_infile_or_exit(infile, argv[3]);
        open_outfile_or_exit(outfile, argv[4]);

        LZ77::decompress(csc2525::SEARCH_BUFFER_SIZE, csc2525::LOOKAHEAD_BUFFER_SIZE, infile, outfile);
        infile.close();
        outfile.close();
    } else {
        std::cerr << "mode has to be wither lz77 or lz4" << std::endl;
        return 1;
    }

    return 0;
}
