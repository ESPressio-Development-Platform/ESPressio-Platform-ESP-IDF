#pragma once
#include <cstddef>
#include <cstdint>
#include <esp_err.h>

enum i2c_addr_bit_len_t { I2C_ADDR_BIT_LEN_7 = 0, I2C_ADDR_BIT_LEN_10 = 1 };
struct i2c_master_bus_t {};
struct i2c_master_dev_t {};
using i2c_master_bus_handle_t = i2c_master_bus_t*;
using i2c_master_dev_handle_t = i2c_master_dev_t*;
struct i2c_device_config_t {
    i2c_addr_bit_len_t dev_addr_length{};
    std::uint16_t device_address{};
    std::uint32_t scl_speed_hz{};
};
inline esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t,
                                            const i2c_device_config_t*,
                                            i2c_master_dev_handle_t* out) {
    static i2c_master_dev_t device;
    *out = &device;
    return ESP_OK;
}
inline esp_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t) { return ESP_OK; }
inline esp_err_t i2c_master_transmit(i2c_master_dev_handle_t,
                                     const std::uint8_t*, std::size_t, int) { return ESP_OK; }
inline esp_err_t i2c_master_receive(i2c_master_dev_handle_t,
                                    std::uint8_t* data, std::size_t size, int) {
    for (std::size_t i = 0; i < size; ++i) data[i] = 0x55U;
    return ESP_OK;
}
inline esp_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t,
                                              const std::uint8_t*, std::size_t,
                                              std::uint8_t* data, std::size_t size,
                                              int) {
    for (std::size_t i = 0; i < size; ++i) data[i] = 0x55U;
    return ESP_OK;
}
