//
// Created by zhujose1 on 2026-02-18.
//

#pragma once

#include <bit>
#include <cstdint>
#include <cstring>
#include <vector>
#include "constants.h"

class HashChain {
    std::vector<size_t> head;
    std::vector<size_t> prev;
    size_t search_buffer_size;
    size_t max_chain_attempts;

    static uint32_t hash3(const char *p) {
        uint32_t x = 0;
        std::memcpy(&x, p, 3);
        return x;
    }

public:
    explicit HashChain(const size_t search_size, const size_t max_chain) :
        head(1ULL << 24, ~0ULL), prev(search_size, ~0ULL), search_buffer_size(search_size),
        max_chain_attempts(max_chain) {}

    // hash using the relative ptr, insert using the absolute pointertr
    void insert(const char *buffer_ptr, const size_t absolute_pos) {
        const uint32_t h = hash3(buffer_ptr);
        prev[absolute_pos % search_buffer_size] = head[h];
        head[h] = absolute_pos;
    }

    /**
     * @param buffer pointer to start of current buffer
     * @param buffer_start_abs absolute file position corresponding to buffer[0]
     * @param current_abs_pos absolute file position we're looking for matches at
     * @param search_start_abs earliest absolute position to search from
     * @param max_len maximum match length
     * @param good_enough_length stop exploring matches after finding match atleast this long
     * @return pair of (offset, length) for longest match found, or (0, 0) if no match
     */
    std::pair<size_t, size_t> find_good_enough_match(const char *buffer, const size_t buffer_start_abs,
                                                     const size_t current_abs_pos, const size_t search_start_abs,
                                                     const size_t max_len, const size_t good_enough_length) {
        if (max_len < csc2525::MIN_MATCH_LENGTH) {
            return {0, 0};
        }

        const size_t current_buf_idx = current_abs_pos - buffer_start_abs;
        const uint32_t h = hash3(buffer + current_buf_idx);

        size_t best_off = 0, best_len = 0;
        size_t attempts = max_chain_attempts;
        size_t candidate_abs_pos = head[h];

        // search the chain starting from most recent
        while (candidate_abs_pos != ~0ULL && attempts--) {
            if (candidate_abs_pos < search_start_abs)
                break;
            if (candidate_abs_pos < buffer_start_abs)
                break; // candidate is no longer in the buffer

            const size_t candidate_buf_idx = candidate_abs_pos - buffer_start_abs;
            size_t len = csc2525::MIN_MATCH_LENGTH; // The 3-byte hash matches exactly by definition

            // Fast 8-byte word comparison for the rest of the match
            while (len + 8 <= max_len) {
                uint64_t curr, prev_val;
                memcpy(&curr, buffer + current_buf_idx + len, 8);
                memcpy(&prev_val, buffer + candidate_buf_idx + len, 8);
                if (curr == prev_val) {
                    len += 8;
                } else {
                    int difference_in;
                    if constexpr (std::endian::native == std::endian::little) {
                        difference_in = __builtin_ctzll(curr ^ prev_val) >> 3;
                    } else {
                        difference_in = __builtin_clzll(curr ^ prev_val) >> 3;
                    }
                    len += difference_in;
                    break;
                }
            }

            // Fallback byte-by-byte for the tail end
            while (len < max_len && buffer[candidate_buf_idx + len] == buffer[current_buf_idx + len]) {
                len++;
            }

            if (len > best_len) {
                best_len = len;
                best_off = current_abs_pos - candidate_abs_pos;
                if (best_len >= good_enough_length || best_len >= max_len)
                    break;
            }

            candidate_abs_pos = prev[candidate_abs_pos % search_buffer_size];
        }
        return {best_off, best_len};
    }
};
