#pragma once
#include <cstddef>
#include <cstdint>
#include <esp_err.h>

using spi_host_device_t = int;
struct spi_device_t {};
using spi_device_handle_t = spi_device_t*;
#define SPI_DEVICE_BIT_LSBFIRST 0x01U
struct spi_device_interface_config_t {
    int clock_speed_hz{};
    std::uint8_t mode{};
    int spics_io_num{-1};
    int queue_size{};
    std::uint32_t flags{};
};
struct spi_transaction_t {
    std::size_t length{};
    const void* tx_buffer{};
    void* rx_buffer{};
};
inline esp_err_t spi_bus_add_device(spi_host_device_t,
                                    const spi_device_interface_config_t*,
                                    spi_device_handle_t* out) {
    static spi_device_t device;
    *out = &device;
    return ESP_OK;
}
inline esp_err_t spi_bus_remove_device(spi_device_handle_t) { return ESP_OK; }
inline esp_err_t spi_device_transmit(spi_device_handle_t, spi_transaction_t* transaction) {
    if (transaction->rx_buffer != nullptr && transaction->tx_buffer != nullptr) {
        auto* rx = static_cast<std::uint8_t*>(transaction->rx_buffer);
        const auto* tx = static_cast<const std::uint8_t*>(transaction->tx_buffer);
        for (std::size_t i = 0; i < transaction->length / 8U; ++i) rx[i] = tx[i];
    }
    return ESP_OK;
}
