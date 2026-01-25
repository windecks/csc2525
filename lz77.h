#include <cstddef>
#include <iosfwd>
#include <vector>

#ifndef CSC2525_LZ77_H
#define CSC2525_LZ77_H

static class LZ77 {
public:
    static std::vector<std::tuple<int, int, char>> compress(size_t window_size, const char *inputFile);
    static void decompress(const char *inputFile, const char *outputFile);
} LZ77;

#endif // CSC2525_LZ77_H
