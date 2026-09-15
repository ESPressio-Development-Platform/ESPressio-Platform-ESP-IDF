#pragma once

#include <cstddef>
#include <cstdint>

#include "esp_err.h"

using esp_partition_type_t = std::uint8_t;
using esp_partition_subtype_t = std::uint8_t;

#define ESP_PARTITION_TYPE_APP 0x00
#define ESP_PARTITION_TYPE_DATA 0x01
#define ESP_PARTITION_TYPE_ANY 0xFF
#define ESP_PARTITION_SUBTYPE_ANY 0xFF

struct esp_partition_t final {
    esp_partition_type_t type{};
    esp_partition_subtype_t subtype{};
    std::uint32_t address{};
    std::size_t size{};
    char label[17]{};
};

struct fake_esp_partition_iterator;
using esp_partition_iterator_t = fake_esp_partition_iterator*;

esp_partition_iterator_t esp_partition_find(
    esp_partition_type_t type,
    esp_partition_subtype_t subtype,
    const char* label);
const esp_partition_t* esp_partition_get(esp_partition_iterator_t iterator);
esp_partition_iterator_t esp_partition_next(esp_partition_iterator_t iterator);
void esp_partition_iterator_release(esp_partition_iterator_t iterator);
