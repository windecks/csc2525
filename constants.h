//
// Created by zhujose1 on 2026-02-18.
//

#pragma once

namespace csc2525 {
    constexpr size_t KB = 1024;
    constexpr size_t BIT_WRITER_BUFFER_SIZE = 4 * KB;

    constexpr size_t MIN_MATCH_LENGTH = 1; // Minimum match length to consider for compression

    constexpr size_t SEARCH_BUFFER_SIZE = 32 * KB; // 32KB search window
    constexpr size_t LOOKAHEAD_BUFFER_SIZE = 258; // Max match length
} // namespace csc2525
