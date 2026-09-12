#pragma once

#include <esp_timer.h>

#include "ESPressio_PlatformClock.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

/**
 * Monotonic microsecond clock backed by ESP-IDF esp_timer_get_time().
 *
 * ESP Timer reports microseconds since its runtime initialization. The source
 * is unaffected by civil-time changes, is documented for task and ISR reads,
 * and restarts its epoch after deep sleep.
 */
class EspTimerClock final
    : public Clock::MonotonicProviderDeclaration<
          Backend,
          1'000'000ULL,
          63U,
          PropertySet<
              PropertyValue<PropertyKey::ClockResolutionNanoseconds,
                            1'000ULL>>,
          CapabilitySet<
              Capability::InterruptReadableClock,
              Capability::HighResolutionClock>> {
public:
    Clock::Tick Now() const noexcept {
        return static_cast<Clock::Tick>(::esp_timer_get_time());
    }

    Clock::Tick NowFromInterrupt() const noexcept {
        return static_cast<Clock::Tick>(::esp_timer_get_time());
    }
};

static_assert(Clock::IsClockSourceV<EspTimerClock>,
              "ESP-IDF EspTimerClock must satisfy the Platform Clock contract");
static_assert(Clock::IsInterruptReadableClockSourceV<EspTimerClock>,
              "ESP-IDF EspTimerClock must satisfy the interrupt-readable Clock contract");

} // namespace ESPressio::Platform::IDF
