# ESPressio-Platform-ESP-IDF

ESP-IDF backend for `ESPressio-Platform`.

This repository owns implementations that genuinely depend on ESP-IDF or Espressif-specific extensions. Generic FreeRTOS behaviour belongs in `ESPressio-Platform-FreeRTOS`; Arduino framework behaviour belongs in `ESPressio-Platform-Arduino`.

The repository is a clean-slate target backend and does not conform to legacy ESPressio platform contracts.

## Clock

`IDF::EspTimerClock` is the first concrete provider. It is backed directly by `esp_timer_get_time()` and provides:

- `Capability::Clock`
- `Capability::MonotonicClock`
- `Capability::InterruptReadableClock`
- `Capability::HighResolutionClock`
- `ClockFrequencyHz = 1,000,000`
- `ClockCounterWidthBits = 63`
- `ClockResolutionNanoseconds = 1,000`

The provider exposes both `Now()` and `NowFromInterrupt()` in the same microsecond tick domain.

The clock epoch is runtime-local: ESP Timer reports elapsed time since its initialization and restarts from zero after deep sleep. No civil-time, timezone, or wall-clock semantics are mixed into the Platform Clock contract.
