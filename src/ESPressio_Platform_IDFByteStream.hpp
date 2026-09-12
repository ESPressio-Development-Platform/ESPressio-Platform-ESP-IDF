#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include <driver/uart.h>
#include <esp_err.h>

#include "ESPressio_PlatformByteStream.hpp"
#include "ESPressio_Platform_IDF.hpp"

namespace ESPressio::Platform::IDF {

template <uart_port_t TPort,
          typename TStreamTag = ESPressio::Platform::ByteStream::DefaultStream>
class UartByteStream final
    : public ESPressio::Platform::ByteStream::ProviderDeclaration<
          Backend,
          TStreamTag> {
    static_assert(static_cast<int>(TPort) >= 0 && static_cast<int>(TPort) < static_cast<int>(UART_NUM_MAX),
                  "ESP-IDF UART port is outside the supported range");
public:
    static constexpr uart_port_t Port = TPort;

    IO::Result Available(std::size_t& count) noexcept {
        count = 0U;
        if (!uart_is_driver_installed(TPort)) return IO::Result::NotInitialized;
        const auto result = uart_get_buffered_data_len(TPort, &count);
        return result == ESP_OK ? IO::Result::Ok : IO::Result::IoError;
    }

    IO::TransferResult Read(IO::MutableBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        if (!uart_is_driver_installed(TPort)) return {IO::Result::NotInitialized, 0U};
        if (buffer.Empty()) return {IO::Result::Ok, 0U};

        constexpr auto maximum = static_cast<std::size_t>(std::numeric_limits<int>::max());
        const auto requested = buffer.Size < maximum ? buffer.Size : maximum;
        const int count = uart_read_bytes(
            TPort,
            buffer.Data,
            static_cast<std::uint32_t>(requested),
            0U);
        if (count < 0) return {IO::Result::IoError, 0U};
        return {IO::Result::Ok, static_cast<std::size_t>(count)};
    }

    IO::TransferResult Write(IO::ConstBuffer buffer) noexcept {
        if (!buffer.IsValid()) return {IO::Result::InvalidArgument, 0U};
        if (!uart_is_driver_installed(TPort)) return {IO::Result::NotInitialized, 0U};
        if (buffer.Empty()) return {IO::Result::Ok, 0U};

        constexpr auto maximum = static_cast<std::size_t>(std::numeric_limits<int>::max());
        const auto requested = buffer.Size < maximum ? buffer.Size : maximum;
        const int count = uart_write_bytes(TPort, buffer.Data, requested);
        if (count < 0) return {IO::Result::IoError, 0U};
        return {IO::Result::Ok, static_cast<std::size_t>(count)};
    }
};

} // namespace ESPressio::Platform::IDF
