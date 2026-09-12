#pragma once

#include <cstddef>
#include <cstdint>

#include <esp_err.h>

using uart_port_t = int;
inline constexpr uart_port_t UART_NUM_0 = 0;
inline constexpr uart_port_t UART_NUM_1 = 1;
inline constexpr uart_port_t UART_NUM_2 = 2;
inline constexpr uart_port_t UART_NUM_MAX = 3;

inline bool uart_is_driver_installed(uart_port_t port) noexcept {
    return port >= 0 && port < UART_NUM_MAX;
}

inline esp_err_t uart_get_buffered_data_len(uart_port_t port, std::size_t* size) noexcept {
    if (!uart_is_driver_installed(port) || size == nullptr) return ESP_ERR_INVALID_ARG;
    *size = 3U;
    return ESP_OK;
}

inline int uart_read_bytes(uart_port_t port, void* buffer, std::uint32_t length, std::uint32_t) noexcept {
    if (!uart_is_driver_installed(port) || (buffer == nullptr && length != 0U)) return -1;
    auto* bytes = static_cast<std::uint8_t*>(buffer);
    for (std::uint32_t i = 0U; i < length; ++i) bytes[i] = static_cast<std::uint8_t>(0x30U + i);
    return static_cast<int>(length);
}

inline int uart_write_bytes(uart_port_t port, const void* buffer, std::size_t size) noexcept {
    if (!uart_is_driver_installed(port) || (buffer == nullptr && size != 0U)) return -1;
    return static_cast<int>(size);
}
