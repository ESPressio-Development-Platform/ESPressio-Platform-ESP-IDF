#pragma once

#include <esp_random.h>

#include "ESPressio_PlatformEntropy.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

/**
 * Entropy/random-byte provider backed by the ESP-IDF hardware RNG surface.
 *
 * The hardware provenance is a compile-time fact. The amount of fresh physical
 * entropy available to the RNG is intentionally not advertised as a static
 * property because ESP-IDF documents that this depends on runtime RF/ADC state.
 */
class HardwareRngEntropy final
    : public ESPressio::Platform::Entropy::ProviderDeclaration<
          Backend,
          CapabilitySet<Capability::HardwareRandomGenerator>> {
public:
    IO::Result Fill(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return IO::Result::InvalidArgument;
        if (buffer.Empty()) return IO::Result::Ok;
        esp_fill_random(buffer.Data, buffer.Size);
        return IO::Result::Ok;
    }
};

static_assert(ESPressio::Platform::Entropy::IsSourceV<HardwareRngEntropy>,
              "ESP-IDF HardwareRngEntropy must satisfy the Platform Entropy contract");

} // namespace ESPressio::Platform::IDF
