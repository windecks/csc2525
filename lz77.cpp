#include "lz77.h"

#include <utility>
#include <vector>

namespace {
    std::pair<size_t, size_t> findLongestMatch(const char *start, const char *current) {
        size_t best_offset = 0;
        size_t best_length = 0;

        for (const char *p = start; p < current; ++p) {
            size_t length = 0;
            while (current[length] != '\0' && p[length] == current[length]) {
                length++;
            }

            if (length > best_length) {
                best_length = length;
                // backwards offset
                best_offset = current - p;
            }
        }
        return {best_offset, best_length};
    }
} // namespace

std::vector<std::tuple<int, int, char>> LZ77::compress(size_t window_size, const char *inputFile) {
    const char *current = inputFile;
    std::vector<std::tuple<int, int, char>> output;
    while (*current != '\0') {
        const char *search_start = std::max(inputFile, current - window_size);

        auto [offset, length] = findLongestMatch(search_start, current);

        const char literal_char = *(current + length);
        output.emplace_back(static_cast<int>(offset), static_cast<int>(length), literal_char);

        current += (length + 1);
        if (literal_char == '\0')
            break;
    }
    return output;
}
