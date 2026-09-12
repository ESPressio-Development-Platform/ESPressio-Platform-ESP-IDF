#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>

#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_err.h>

#include "ESPressio_PlatformBus.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

namespace Detail {
inline IO::Result MapEspError(esp_err_t error) noexcept {
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
} // namespace Detail

template <typename TDeviceTag,
          std::uint16_t TAddress,
          std::uint32_t TFrequencyHz = 400'000U,
          I2C::AddressWidth TAddressWidth = I2C::AddressWidth::Bits7,
          int TTimeoutMilliseconds = 1'000>
class I2CDevice final
    : public I2C::DeviceProviderDeclaration<
          Backend, TDeviceTag, TAddress, TAddressWidth, TFrequencyHz> {
public:
    constexpr I2CDevice() noexcept = default;
    explicit I2CDevice(i2c_master_bus_handle_t bus) noexcept { (void)Attach(bus); }

    I2CDevice(const I2CDevice&) = delete;
    I2CDevice& operator=(const I2CDevice&) = delete;

    I2CDevice(I2CDevice&& other) noexcept : device_(other.device_) {
        other.device_ = nullptr;
    }

    I2CDevice& operator=(I2CDevice&& other) noexcept {
        if (this != &other) {
            (void)Detach();
            device_ = other.device_;
            other.device_ = nullptr;
        }
        return *this;
    }

    ~I2CDevice() { (void)Detach(); }

    IO::Result Attach(i2c_master_bus_handle_t bus) noexcept {
        if (bus == nullptr) return IO::Result::InvalidArgument;
        if (device_ != nullptr) return IO::Result::Busy;

        i2c_device_config_t configuration{};
        configuration.dev_addr_length =
            TAddressWidth == I2C::AddressWidth::Bits10
                ? I2C_ADDR_BIT_LEN_10
                : I2C_ADDR_BIT_LEN_7;
        configuration.device_address = TAddress;
        configuration.scl_speed_hz = TFrequencyHz;

        return Detail::MapEspError(
            i2c_master_bus_add_device(bus, &configuration, &device_));
    }

    IO::Result Detach() noexcept {
        if (device_ == nullptr) return IO::Result::Ok;
        const auto handle = device_;
        device_ = nullptr;
        return Detail::MapEspError(i2c_master_bus_rm_device(handle));
    }

    bool IsAttached() const noexcept { return device_ != nullptr; }
    i2c_master_dev_handle_t NativeHandle() const noexcept { return device_; }

    IO::Result Write(IO::ConstBuffer buffer) noexcept {
        if (!buffer.IsValid()) return IO::Result::InvalidArgument;
        if (device_ == nullptr) return IO::Result::NotInitialized;
        if (buffer.Empty()) return IO::Result::Ok;
        return Detail::MapEspError(i2c_master_transmit(
            device_, buffer.Data, buffer.Size, TTimeoutMilliseconds));
    }

    IO::Result Read(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return IO::Result::InvalidArgument;
        if (device_ == nullptr) return IO::Result::NotInitialized;
        if (buffer.Empty()) return IO::Result::Ok;
        return Detail::MapEspError(i2c_master_receive(
            device_, buffer.Data, buffer.Size, TTimeoutMilliseconds));
    }

    IO::Result WriteRead(IO::ConstBuffer writeBuffer,
                         IO::MutableBuffer readBuffer) noexcept {
        if (!writeBuffer.IsValid() || !readBuffer.IsValid())
            return IO::Result::InvalidArgument;
        if (device_ == nullptr) return IO::Result::NotInitialized;
        if (writeBuffer.Empty()) return Read(readBuffer);
        if (readBuffer.Empty()) return Write(writeBuffer);
        return Detail::MapEspError(i2c_master_transmit_receive(
            device_,
            writeBuffer.Data,
            writeBuffer.Size,
            readBuffer.Data,
            readBuffer.Size,
            TTimeoutMilliseconds));
    }

private:
    i2c_master_dev_handle_t device_{nullptr};
};

template <typename TDeviceTag,
          std::uint32_t TFrequencyHz,
          SPI::Mode TMode = SPI::Mode::Mode0,
          SPI::BitOrder TBitOrder = SPI::BitOrder::MostSignificantFirst>
class SPIDevice final
    : public SPI::DeviceProviderDeclaration<
          Backend, TDeviceTag, TFrequencyHz, TMode, TBitOrder> {
public:
    constexpr SPIDevice() noexcept = default;
    SPIDevice(const SPIDevice&) = delete;
    SPIDevice& operator=(const SPIDevice&) = delete;

    SPIDevice(SPIDevice&& other) noexcept : device_(other.device_) {
        other.device_ = nullptr;
    }

    SPIDevice& operator=(SPIDevice&& other) noexcept {
        if (this != &other) {
            (void)Detach();
            device_ = other.device_;
            other.device_ = nullptr;
        }
        return *this;
    }

    ~SPIDevice() { (void)Detach(); }

    IO::Result Attach(spi_host_device_t host, int chipSelectPin = -1) noexcept {
        if (device_ != nullptr) return IO::Result::Busy;

        spi_device_interface_config_t configuration{};
        configuration.clock_speed_hz = static_cast<int>(TFrequencyHz);
        configuration.mode = static_cast<std::uint8_t>(TMode);
        configuration.spics_io_num = chipSelectPin;
        configuration.queue_size = 1;
        if constexpr (TBitOrder == SPI::BitOrder::LeastSignificantFirst)
            configuration.flags |= SPI_DEVICE_BIT_LSBFIRST;

        return Detail::MapEspError(
            spi_bus_add_device(host, &configuration, &device_));
    }

    IO::Result Detach() noexcept {
        if (device_ == nullptr) return IO::Result::Ok;
        const auto handle = device_;
        device_ = nullptr;
        return Detail::MapEspError(spi_bus_remove_device(handle));
    }

    bool IsAttached() const noexcept { return device_ != nullptr; }
    spi_device_handle_t NativeHandle() const noexcept { return device_; }

    IO::Result Transfer(const std::uint8_t* transmit,
                        std::uint8_t* receive,
                        std::size_t size) noexcept {
        if (size != 0U && transmit == nullptr && receive == nullptr)
            return IO::Result::InvalidArgument;
        if (device_ == nullptr) return IO::Result::NotInitialized;
        if (size == 0U) return IO::Result::Ok;

        spi_transaction_t transaction{};
        transaction.length = size * 8U;
        transaction.tx_buffer = transmit;
        transaction.rx_buffer = receive;
        return Detail::MapEspError(spi_device_transmit(device_, &transaction));
    }

private:
    spi_device_handle_t device_{nullptr};
};

} // namespace ESPressio::Platform::IDF
