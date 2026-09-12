#include "ESPressio_Platform_IDF.hpp"

using namespace ESPressio::Platform;

struct RTCDeviceTag {};
struct SensorTag {};

using I2CDev = IDF::I2CDevice<RTCDeviceTag, 0x68U, 400'000U>;
using SPIDev = IDF::SPIDevice<SensorTag,
                              8'000'000U,
                              SPI::Mode::Mode3,
                              SPI::BitOrder::MostSignificantFirst>;
using Gpio = IDF::GPIOController<>;

static_assert(I2C::IsDeviceV<I2CDev, RTCDeviceTag>);
static_assert(SPI::IsDeviceV<SPIDev, SensorTag>);
static_assert(GPIO::IsControllerV<Gpio>);
static_assert(GPIO::IsInterruptControllerV<Gpio>);
static_assert(Gpio::PlatformCapabilities::template Contains<
              Capability::GPIOOpenDrain<GPIO::DefaultDomain>>);
static_assert(Gpio::PlatformCapabilities::template Contains<
              Capability::GPIOPullDown<GPIO::DefaultDomain>>);

void Handler(void*) noexcept {}

int main() {
    i2c_master_bus_t busStorage;
    I2CDev i2c;
    if (i2c.Attach(&busStorage) != IO::Result::Ok) return 1;
    std::uint8_t writeData[2]{1U, 2U};
    std::uint8_t readData[2]{};
    if (i2c.WriteRead(IO::Bytes(writeData), IO::Bytes(readData)) != IO::Result::Ok)
        return 2;

    SPIDev spi;
    if (spi.Attach(0) != IO::Result::Ok) return 3;
    if (spi.Transfer(writeData, readData, 2U) != IO::Result::Ok) return 4;

    Gpio gpio;
    GPIO::Configuration configuration{};
    configuration.DirectionMode = GPIO::Direction::InputOutput;
    configuration.PullMode = GPIO::Pull::Up;
    configuration.Output = GPIO::OutputMode::OpenDrain;
    if (gpio.Configure(1U, configuration) != IO::Result::Ok) return 5;
    if (gpio.InitializeInterrupts() != IO::Result::Ok) return 6;
    if (gpio.AttachInterrupt(1U, GPIO::InterruptTrigger::RisingEdge, Handler, nullptr) !=
        IO::Result::Ok) return 7;
    return gpio.DetachInterrupt(1U) == IO::Result::Ok ? 0 : 8;
}
