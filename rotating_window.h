#pragma once

#include <array>
#include <memory>
#include <utility>
#include <cstdint>

template<uint32_t C, uint32_t N>
class RotatingWindow {
public:
    RotatingWindow() {
        static_assert(C > 1);
        for (auto &ptr: windows) {
            ptr = std::make_unique<char[]>(N);
        }
    }

    auto operator[](const unsigned index) {
        unsigned adjusted_index = (first_window + index) % C;
        return windows[adjusted_index].get();
    }

    [[nodiscard]] std::pair<char *, uint32_t> write_at() {
        return {windows[last_window].get() + last_window_filled, N - last_window_filled};
    }

    [[nodiscard]] std::pair<char *, uint32_t> read_from(const uint32_t offset) {
        if (offset <= last_window_filled) {
            const auto idx = last_window_filled - offset;
            return {windows[last_window].get() + idx, last_window_filled - idx};
        }

        const uint32_t overfill = offset - last_window_filled - 1;
        const uint32_t windows_back = overfill / N + 1;
        const uint32_t window_idx = (last_window + C - (windows_back % C)) % C;
        const uint32_t dist_from_end = overfill % N;
        const uint32_t window_off = N - dist_from_end - 1;

        return {windows[window_idx].get() + window_off, N - window_off};
    }

    void mark_written(const unsigned bytes) {
        last_window_filled += bytes;
        if (last_window_filled >= N) {
            last_window_filled = 0;
            last_window = (last_window + 1) % C;
            if (active_windows < C) {
                ++active_windows;
            } else {
                first_window = (first_window + 1) % C;
            }
        }
    }

    [[nodiscard]] uint32_t num_active_windows() const { return active_windows; }

private:
    std::array<std::unique_ptr<char[]>, C> windows;
    uint32_t active_windows = 1;
    uint32_t first_window = 0;
    uint32_t last_window = 0;
    uint32_t last_window_filled = 0;
};
