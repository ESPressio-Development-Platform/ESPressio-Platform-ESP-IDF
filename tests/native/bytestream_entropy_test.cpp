#include <cstddef>
#include <cstdint>

#include "ESPressio_Platform_IDF.hpp"

namespace {

struct ConsoleTag {};
using Uart = ESPressio::Platform::IDF::UartByteStream<UART_NUM_1, ConsoleTag>;
using Entropy = ESPressio::Platform::IDF::HardwareRngEntropy;

static_assert(ESPressio::Platform::ByteStream::IsStreamV<Uart, ConsoleTag>);
static_assert(ESPressio::Platform::Entropy::IsSourceV<Entropy>);
static_assert(Entropy::PlatformCapabilities::template Contains<
              ESPressio::Platform::Capability::HardwareRandomGenerator>);

} // namespace

int main() {
    Uart stream;
    std::size_t available = 0U;
    if (stream.Available(available) != ESPressio::Platform::IO::Result::Ok || available != 3U)
        return 1;

    std::uint8_t received[3]{};
    const auto read = stream.Read(ESPressio::Platform::IO::Bytes(received));
    if (!read.Completed(sizeof(received))) return 2;
    if (received[0] != 0x30U || received[2] != 0x32U) return 3;

    const std::uint8_t transmit[2]{0x11U, 0x22U};
    const auto write = stream.Write(ESPressio::Platform::IO::Bytes(transmit));
    if (!write.Completed(sizeof(transmit))) return 4;

    Entropy entropy;
    std::uint8_t random[4]{};
    if (entropy.Fill(ESPressio::Platform::IO::Bytes(random)) != ESPressio::Platform::IO::Result::Ok)
        return 5;
    return random[0] == 0xA0U && random[3] == 0xA3U ? 0 : 6;
}
