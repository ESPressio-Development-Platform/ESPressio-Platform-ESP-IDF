#include "ESPressio_Platform_IDF.hpp"

extern "C" int64_t esp_timer_get_time(void) {
    return 987654;
}

using Clock = ESPressio::Platform::IDF::EspTimerClock;

static_assert(ESPressio::Platform::Clock::IsClockSourceV<Clock>);
static_assert(ESPressio::Platform::Clock::IsInterruptReadableClockSourceV<Clock>);
static_assert(ESPressio::Platform::Clock::FrequencyHz<Clock> == 1'000'000ULL);
static_assert(ESPressio::Platform::Clock::CounterWidthBits<Clock> == 63U);
static_assert(ESPressio::Platform::Clock::HasResolution<Clock>);

int main() {
    Clock clock;
    return clock.Now() == 987654ULL &&
                   clock.NowFromInterrupt() == 987654ULL
               ? 0
               : 1;
}
