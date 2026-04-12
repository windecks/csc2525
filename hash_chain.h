//
// Created by zhujose1 on 2026-02-18.
//

#pragma once

#include <deque>
#include <unordered_map>
#include "constants.h"

class HashChain {
    std::unordered_map<uint32_t, std::deque<size_t>> chains;
    size_t search_buffer_size;

    static uint32_t hash3(const char *p) {
        uint32_t x = 0;
        std::memcpy(&x, p, 3);
        return x;
    }

public:
    explicit HashChain(const size_t search_size) : search_buffer_size(search_size) {}

    // hash using the relative ptr, insert using the absolute pointertr
    void insert(const char *buffer_ptr, const size_t absolute_pos) {
        const uint32_t h = hash3(buffer_ptr);
        chains[h].push_back(absolute_pos);
    }

    void evict_old(const size_t current_abs_pos) {
        if (current_abs_pos < search_buffer_size)
            return;
        const size_t evict_boundary = current_abs_pos - search_buffer_size;

        for (auto &[hash, chain]: chains) {
            while (!chain.empty() && chain.front() < evict_boundary) {
                chain.pop_front();
            }
        }
    }

    /**
     * @param buffer pointer to start of current buffer
     * @param buffer_start_abs absolute file position corresponding to buffer[0]
     * @param current_abs_pos absolute file position we're looking for matches at
     * @param search_start_abs earliest absolute position to search from
     * @param max_len maximum match length
     * @return pair of (offset, length) for longest match found, or (0, 0) if no match
     */
    std::pair<size_t, size_t> find_longest_match(const char *buffer, const size_t buffer_start_abs,
                                                 const size_t current_abs_pos, const size_t search_start_abs,
                                                 const size_t max_len) {
        if (max_len < csc2525::MIN_MATCH_LENGTH) {
            return {0, 0};
        }

        const size_t current_buf_idx = current_abs_pos - buffer_start_abs;
        const uint32_t h = hash3(buffer + current_buf_idx);
        const auto it = chains.find(h);
        if (it == chains.end()) {
            return {0, 0};
        }

        size_t best_off = 0, best_len = 0;
        const auto &chain = it->second;

        // search the chain starting from most recent
        for (auto rit = chain.rbegin(); rit != chain.rend(); ++rit) {
            const size_t candidate_abs_pos = *rit;
            if (candidate_abs_pos < search_start_abs)
                break;
            if (candidate_abs_pos < buffer_start_abs)
                break; // candidate is no longer in the buffer
            if (candidate_abs_pos >= current_abs_pos)
                continue;

            const size_t candidate_buf_idx = candidate_abs_pos - buffer_start_abs;
            size_t len = 0;
            while (len < max_len && buffer[candidate_buf_idx + len] == buffer[current_buf_idx + len]) {
                len++;
            }

            if (len > best_len) {
                best_len = len;
                best_off = current_abs_pos - candidate_abs_pos;
                if (best_len == max_len)
                    break;
            }
        }
        return {best_off, best_len};
    }
};
