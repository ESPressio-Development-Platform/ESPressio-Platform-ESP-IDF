#pragma once
#include <cstdint>
#include <esp_err.h>

using gpio_num_t = int;
using gpio_isr_t = void (*)(void*);
enum gpio_mode_t {
    GPIO_MODE_INPUT = 1,
    GPIO_MODE_OUTPUT = 2,
    GPIO_MODE_OUTPUT_OD = 3,
    GPIO_MODE_INPUT_OUTPUT_OD = 4,
    GPIO_MODE_INPUT_OUTPUT = 5
};
enum gpio_pullup_t { GPIO_PULLUP_DISABLE = 0, GPIO_PULLUP_ENABLE = 1 };
enum gpio_pulldown_t { GPIO_PULLDOWN_DISABLE = 0, GPIO_PULLDOWN_ENABLE = 1 };
enum gpio_int_type_t {
    GPIO_INTR_DISABLE = 0,
    GPIO_INTR_POSEDGE,
    GPIO_INTR_NEGEDGE,
    GPIO_INTR_ANYEDGE,
    GPIO_INTR_LOW_LEVEL,
    GPIO_INTR_HIGH_LEVEL
};
struct gpio_config_t {
    std::uint64_t pin_bit_mask{};
    gpio_mode_t mode{};
    gpio_pullup_t pull_up_en{};
    gpio_pulldown_t pull_down_en{};
    gpio_int_type_t intr_type{};
};
#define GPIO_IS_VALID_GPIO(pin) ((pin) < 64U)
#define GPIO_IS_VALID_OUTPUT_GPIO(pin) ((pin) < 64U)
inline esp_err_t gpio_config(const gpio_config_t*) { return ESP_OK; }
inline esp_err_t gpio_set_level(gpio_num_t, std::uint32_t) { return ESP_OK; }
inline int gpio_get_level(gpio_num_t) { return 0; }
inline esp_err_t gpio_install_isr_service(int) { return ESP_OK; }
inline esp_err_t gpio_set_intr_type(gpio_num_t, gpio_int_type_t) { return ESP_OK; }
inline esp_err_t gpio_isr_handler_add(gpio_num_t, gpio_isr_t, void*) { return ESP_OK; }
inline esp_err_t gpio_isr_handler_remove(gpio_num_t) { return ESP_OK; }
