# ESPressio-Platform-ESP-IDF

ESP-IDF backend for `ESPressio-Platform`.

This repository owns implementations that genuinely depend on ESP-IDF or Espressif-specific extensions. Generic FreeRTOS behaviour belongs in `ESPressio-Platform-FreeRTOS`; Arduino framework behaviour belongs in `ESPressio-Platform-Arduino`.

The repository is a clean-slate target backend and does not conform to legacy ESPressio platform contracts.

## Clock

`IDF::EspTimerClock` is backed directly by `esp_timer_get_time()` and provides monotonic, high-resolution, interrupt-readable Platform Clock semantics with a 1 MHz tick scale, 63-bit counter domain and 1 microsecond declared resolution.

The clock epoch is runtime-local: no civil-time, timezone, or wall-clock semantics are mixed into the Platform Clock contract.

## I2C

`IDF::I2CDevice<Tag, Address, Frequency, AddressWidth, Timeout>` is a bound endpoint using the current ESP-IDF master bus/device API. The selected address, 7/10-bit address width and target bus frequency are compile-time Platform properties.

Capability selection is static, while native driver availability is explicit runtime state: `Attach(i2c_master_bus_handle_t)` installs the device on an already-created IDF bus and can fail normally. `Detach()` releases only that device handle. Write, read and repeated-START write/read map directly to the native device operations.

## SPI

`IDF::SPIDevice<Tag, Frequency, Mode, BitOrder>` uses the ESP-IDF SPI master device model. `Attach(host, chipSelectPin)` adds the endpoint to an already-initialized SPI bus; passing `-1` leaves chip-select ownership external so a composed GPIO provider can own CS. The endpoint is move-only and releases only its own native device handle.

## GPIO

`IDF::GPIOController<DomainTag>` provides the generic GPIO contract and truthfully advertises the stronger ESP-IDF capabilities available through the native GPIO driver: pull-up, pull-down, open-drain, bidirectional operation and per-pin interrupts.

Interrupt initialization uses ESP-IDF's shared ISR service. An already-installed service is treated as available, and an individual GPIO provider never uninstalls that process-global service. Interrupt handlers can then be attached/detached per pin through the generic Platform contract.

As with all Platform providers, structural support is a compile-time fact while runtime driver/resource failures remain runtime results.

## ByteStream

`IDF::UartByteStream<Port, Tag>` adapts an already-installed ESP-IDF UART driver to the tagged Platform ByteStream contract. The adapter does not install, configure, own, or delete the UART driver, keeping baud rate, pins, ring-buffer sizing and ISR resource ownership explicit outside the generic stream abstraction.

`Available()` reports the current RX ring-buffer population. `Read()` uses a zero-tick wait and therefore only consumes bytes available at call time. `Write()` delegates to `uart_write_bytes()` and reports the number of bytes accepted; its blocking behaviour remains the behaviour selected when the native UART driver was configured.

## Entropy

`IDF::HardwareRngEntropy` implements `Capability::Entropy` using `esp_fill_random()` and additionally advertises `Capability::HardwareRandomGenerator` to describe its hardware provenance.

That feature does **not** claim a fixed entropy strength. ESP-IDF documents that continuous true-random output depends on runtime conditions such as an active RF subsystem or explicitly enabled hardware entropy source. Platform therefore leaves cryptographic-strength/entropy-bit claims out of the static provider metadata rather than overstating a runtime guarantee.
