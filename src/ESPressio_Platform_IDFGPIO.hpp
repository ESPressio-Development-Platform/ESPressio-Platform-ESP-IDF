#pragma once

#include <cstdint>

#include <driver/gpio.h>
#include <esp_err.h>

#include "ESPressio_PlatformGPIO.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

namespace GPIODetail {
inline IO::Result Map(esp_err_t error) noexcept {
    switch (error) {
        case ESP_OK: return IO::Result::Ok;
        case ESP_ERR_INVALID_ARG: return IO::Result::InvalidArgument;
        case ESP_ERR_INVALID_STATE: return IO::Result::Busy;
        case ESP_ERR_TIMEOUT: return IO::Result::Timeout;
        case ESP_ERR_NOT_FOUND: return IO::Result::NoDevice;
        case ESP_ERR_NO_MEM: return IO::Result::ResourceExhausted;
        default: return IO::Result::IoError;
    }
}

constexpr gpio_int_type_t InterruptType(GPIO::InterruptTrigger trigger) noexcept {
    switch (trigger) {
        case GPIO::InterruptTrigger::RisingEdge: return GPIO_INTR_POSEDGE;
        case GPIO::InterruptTrigger::FallingEdge: return GPIO_INTR_NEGEDGE;
        case GPIO::InterruptTrigger::AnyEdge: return GPIO_INTR_ANYEDGE;
        case GPIO::InterruptTrigger::LowLevel: return GPIO_INTR_LOW_LEVEL;
        case GPIO::InterruptTrigger::HighLevel: return GPIO_INTR_HIGH_LEVEL;
    }
    return GPIO_INTR_DISABLE;
}
} // namespace GPIODetail

template <typename TDomainTag = GPIO::DefaultDomain>
class GPIOController final
    : public GPIO::ProviderDeclaration<
          Backend,
          TDomainTag,
          CapabilitySet<
              Capability::GPIOPullUp<TDomainTag>,
              Capability::GPIOPullDown<TDomainTag>,
              Capability::GPIOOpenDrain<TDomainTag>,
              Capability::GPIOBidirectional<TDomainTag>,
              Capability::GPIOInterrupts<TDomainTag>>> {
public:
    IO::Result Configure(GPIO::Pin pin, const GPIO::Configuration& configuration) noexcept {
        if (!GPIO_IS_VALID_GPIO(pin)) return IO::Result::InvalidArgument;

        gpio_mode_t mode{};
        switch (configuration.DirectionMode) {
            case GPIO::Direction::Input:
                mode = GPIO_MODE_INPUT;
                break;
            case GPIO::Direction::Output:
                if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return IO::Result::InvalidArgument;
                mode = configuration.Output == GPIO::OutputMode::OpenDrain
                           ? GPIO_MODE_OUTPUT_OD
                           : GPIO_MODE_OUTPUT;
                break;
            case GPIO::Direction::InputOutput:
                if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return IO::Result::InvalidArgument;
                mode = configuration.Output == GPIO::OutputMode::OpenDrain
                           ? GPIO_MODE_INPUT_OUTPUT_OD
                           : GPIO_MODE_INPUT_OUTPUT;
                break;
        }

        gpio_config_t native{};
        native.pin_bit_mask = (std::uint64_t{1} << pin);
        native.mode = mode;
        native.pull_up_en =
            (configuration.PullMode == GPIO::Pull::Up ||
             configuration.PullMode == GPIO::Pull::UpDown)
                ? GPIO_PULLUP_ENABLE
                : GPIO_PULLUP_DISABLE;
        native.pull_down_en =
            (configuration.PullMode == GPIO::Pull::Down ||
             configuration.PullMode == GPIO::Pull::UpDown)
                ? GPIO_PULLDOWN_ENABLE
                : GPIO_PULLDOWN_DISABLE;
        native.intr_type = GPIO_INTR_DISABLE;

        if (configuration.ApplyInitialLevel &&
            configuration.DirectionMode != GPIO::Direction::Input) {
            const auto result = gpio_set_level(
                static_cast<gpio_num_t>(pin),
                configuration.InitialLevel == GPIO::Level::High ? 1U : 0U);
            if (result != ESP_OK) return GPIODetail::Map(result);
        }

        return GPIODetail::Map(gpio_config(&native));
    }

    IO::Result Read(GPIO::Pin pin, GPIO::Level& level) noexcept {
        if (!GPIO_IS_VALID_GPIO(pin)) return IO::Result::InvalidArgument;
        level = gpio_get_level(static_cast<gpio_num_t>(pin)) == 0
                    ? GPIO::Level::Low
                    : GPIO::Level::High;
        return IO::Result::Ok;
    }

    IO::Result Write(GPIO::Pin pin, GPIO::Level level) noexcept {
        if (!GPIO_IS_VALID_OUTPUT_GPIO(pin)) return IO::Result::InvalidArgument;
        return GPIODetail::Map(gpio_set_level(
            static_cast<gpio_num_t>(pin), level == GPIO::Level::High ? 1U : 0U));
    }

    IO::Result InitializeInterrupts() noexcept {
        const auto result = gpio_install_isr_service(0);
        // The ISR service is process-global; an already-installed service is
        // usable by this controller and must not be torn down here.
        return result == ESP_ERR_INVALID_STATE ? IO::Result::Ok : GPIODetail::Map(result);
    }

    IO::Result AttachInterrupt(GPIO::Pin pin,
                               GPIO::InterruptTrigger trigger,
                               GPIO::InterruptHandler handler,
                               void* context) noexcept {
        if (!GPIO_IS_VALID_GPIO(pin) || handler == nullptr)
            return IO::Result::InvalidArgument;
        const auto gpio = static_cast<gpio_num_t>(pin);
        auto result = gpio_set_intr_type(gpio, GPIODetail::InterruptType(trigger));
        if (result != ESP_OK) return GPIODetail::Map(result);
        return GPIODetail::Map(gpio_isr_handler_add(
            gpio, reinterpret_cast<gpio_isr_t>(handler), context));
    }

    IO::Result DetachInterrupt(GPIO::Pin pin) noexcept {
        if (!GPIO_IS_VALID_GPIO(pin)) return IO::Result::InvalidArgument;
        return GPIODetail::Map(gpio_isr_handler_remove(static_cast<gpio_num_t>(pin)));
    }
};

} // namespace ESPressio::Platform::IDF
