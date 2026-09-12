# ESPressio-Platform-ESP-IDF

ESP-IDF backend for `ESPressio-Platform`.

This repository owns implementations that genuinely depend on ESP-IDF or Espressif-specific extensions. Generic FreeRTOS behaviour belongs in `ESPressio-Platform-FreeRTOS`; Arduino framework behaviour belongs in `ESPressio-Platform-Arduino`.

The repository is a clean-slate target backend and does not conform to legacy ESPressio platform contracts.

## Clock

`IDF::EspTimerClock` is backed directly by `esp_timer_get_time()` and provides monotonic, high-resolution, interrupt-readable Platform Clock semantics with a 1 MHz tick scale, 63-bit counter domain and 1 microsecond declared resolution.

## I2C

`IDF::I2CDevice<Tag, Address, Frequency, AddressWidth, Timeout>` is a bound endpoint using the ESP-IDF master bus/device API. Address, address width and target bus frequency are compile-time Platform properties; native driver availability remains explicit runtime state through `Attach()` and `Detach()`.

## SPI

`IDF::SPIDevice<Tag, Frequency, Mode, BitOrder>` uses the ESP-IDF SPI master device model. The endpoint attaches to an already-initialized SPI bus and may leave chip-select ownership to a composed GPIO provider.

## GPIO

`IDF::GPIOController<DomainTag>` provides the generic GPIO contract and advertises the stronger native capabilities available through the ESP-IDF GPIO driver: pull-up, pull-down, open-drain, bidirectional operation and per-pin interrupts.

## ByteStream

`IDF::UartByteStream<Port, Tag>` adapts an already-installed ESP-IDF UART driver to the tagged Platform ByteStream contract. It does not install, configure, own, or delete the UART driver.

## Entropy

`IDF::HardwareRngEntropy` implements `Capability::Entropy` using the ESP-IDF hardware random fill API and advertises `Capability::HardwareRandomGenerator` as a provenance feature. The generic contract does not invent a fixed static quality claim for runtime-dependent physical entropy conditions.

## Synchronization and Queue

This backend intentionally does not duplicate the generic FreeRTOS Synchronization and Queue providers. ESP-IDF uses FreeRTOS for the semaphore and queue primitives covered by these contracts, and portable FreeRTOS behaviour belongs in `ESPressio-Platform-FreeRTOS`.

An ESP-IDF composition can therefore select `FreeRTOS::StaticSynchronizationProvider` and `FreeRTOS::StaticQueueProvider` alongside IDF-specific Clock, GPIO, bus, UART and Entropy providers. This keeps ownership explicit and avoids two provider families around the same kernel mechanisms.

Future IDF-specific extensions belong here only when they provide genuinely different semantics or resource capabilities; this repository will not introduce aliases merely to mirror the portable FreeRTOS implementation.
