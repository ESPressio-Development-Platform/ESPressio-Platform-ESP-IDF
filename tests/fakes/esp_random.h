#pragma once

#include <cstddef>
#include <cstdint>

inline void esp_fill_random(void* buffer, std::size_t length) noexcept {
    auto* bytes = static_cast<std::uint8_t*>(buffer);
    for (std::size_t i = 0U; i < length; ++i)
        bytes[i] = static_cast<std::uint8_t>(0xA0U + (i & 0x0FU));
}
