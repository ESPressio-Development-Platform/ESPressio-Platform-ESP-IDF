# ESPressio-Platform-ESP-IDF

ESP-IDF backend for `ESPressio-Platform`.

This repository owns implementations that genuinely depend on ESP-IDF or Espressif-specific extensions. Generic FreeRTOS behaviour belongs in `ESPressio-Platform-FreeRTOS`; Arduino framework behaviour belongs in `ESPressio-Platform-Arduino`.

The repository is being built as a clean-slate target backend and does not conform to legacy ESPressio platform contracts.

The initial capability groundwork defines only the distinct `ESPressio::Platform::IDF::Backend` identity and the `IDF::ProviderDeclaration` convenience alias. It deliberately makes no service-capability claim until a corresponding ESP-IDF provider is implemented and validated.
