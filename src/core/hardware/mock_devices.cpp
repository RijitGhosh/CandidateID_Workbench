#include "mock_devices.hpp"

namespace aether::hardware {

const char* toString(DeviceKind k) {
    switch (k) {
        case DeviceKind::Microcontroller: return "Microcontroller";
        case DeviceKind::Fpga: return "FPGA";
        case DeviceKind::JtagAdapter: return "JTAG adapter";
    }
    return "?";
}
const char* toString(DeviceState s) {
    switch (s) {
        case DeviceState::Undiscovered: return "Undiscovered";
        case DeviceState::Disconnected: return "Disconnected";
        case DeviceState::Connected: return "Connected";
        case DeviceState::Faulted: return "Faulted";
    }
    return "?";
}
const char* toString(ErrorCode c) {
    switch (c) {
        case ErrorCode::None: return "None";
        case ErrorCode::NotDiscovered: return "NotDiscovered";
        case ErrorCode::NotConnected: return "NotConnected";
        case ErrorCode::AlreadyConnected: return "AlreadyConnected";
        case ErrorCode::Timeout: return "Timeout";
        case ErrorCode::DeviceFaulted: return "DeviceFaulted";
    }
    return "?";
}

MockDevice::MockDevice(DeviceKind kind, std::string id, std::string modelsBoard)
    : info_{kind, std::move(id), std::move(modelsBoard), true} {}

Status MockDevice::discover() {
    if (state_ == DeviceState::Undiscovered) state_ = DeviceState::Disconnected;
    return {};
}

Status MockDevice::capabilities(Capabilities& out) const {
    if (state_ == DeviceState::Undiscovered) return {ErrorCode::NotDiscovered, "run discovery first"};
    out = makeCapabilities();
    return {};
}

Status MockDevice::connect() {
    switch (state_) {
        case DeviceState::Undiscovered: return {ErrorCode::NotDiscovered, "run discovery first"};
        case DeviceState::Connected: return {ErrorCode::AlreadyConnected, "already connected (simulated)"};
        case DeviceState::Faulted: return {ErrorCode::DeviceFaulted, "device is faulted; call recover() first"};
        case DeviceState::Disconnected: break;
    }
    state_ = DeviceState::Connected;
    return {};
}

Status MockDevice::disconnect() {
    if (state_ == DeviceState::Connected || state_ == DeviceState::Faulted) {
        state_ = DeviceState::Disconnected;
        timeoutArmed_ = false;
        return {};
    }
    return {ErrorCode::NotConnected, "device is not connected"};
}

Status MockDevice::readTelemetry(Telemetry& out) {
    if (state_ == DeviceState::Faulted) return {ErrorCode::DeviceFaulted, "device is faulted after a timeout; call recover()"};
    if (state_ != DeviceState::Connected) return {ErrorCode::NotConnected, "device is not connected"};
    if (timeoutArmed_) {
        timeoutArmed_ = false;
        state_ = DeviceState::Faulted;
        return {ErrorCode::Timeout, "simulated telemetry timeout (no reply within the simulated deadline)"};
    }
    out = Telemetry{};
    out.deviceId = info_.id;
    out.sequence = seq_;
    fillTelemetry(out, seq_);
    ++seq_;
    return {};
}

Status MockDevice::recover() {
    if (state_ != DeviceState::Faulted) return {ErrorCode::NotConnected, "nothing to recover: device is not faulted"};
    timeoutArmed_ = false;
    state_ = DeviceState::Connected;
    return {};
}

namespace {

class McuMock final : public MockDevice {
public:
    McuMock() : MockDevice(DeviceKind::Microcontroller, "mock-mcu-0", "ST NUCLEO-F446RE (STM32F446RET6)") {}
protected:
    Capabilities makeCapabilities() const override {
        return {{"USB (ST-LINK virtual COM port)", "SWD debug", "GPIO"}, {"UART telemetry", "simulated ADC channel"}};
    }
    void fillTelemetry(Telemetry& t, std::uint64_t s) const override {
        t.channels = {{"sim_uptime_ticks", static_cast<double>(s * 100)}, {"sim_adc_counts", 2048.0 + static_cast<double>(s % 16)}};
    }
};

class FpgaMock final : public MockDevice {
public:
    FpgaMock() : MockDevice(DeviceKind::Fpga, "mock-fpga-0", "Digilent Arty A7-35T (Artix-7 XC7A35T)") {}
protected:
    Capabilities makeCapabilities() const override {
        return {{"USB-UART (FTDI)", "USB-JTAG (onboard)", "Pmod GPIO"}, {"bitstream status", "simulated clock counter"}};
    }
    void fillTelemetry(Telemetry& t, std::uint64_t s) const override {
        t.channels = {{"sim_configured", 1.0}, {"sim_clock_counter", static_cast<double>(s * 1000)}};
    }
};

class JtagMock final : public MockDevice {
public:
    JtagMock() : MockDevice(DeviceKind::JtagAdapter, "mock-jtag-0", "Digilent JTAG adapter (see docs/embedded_design.md)") {}
protected:
    Capabilities makeCapabilities() const override {
        return {{"USB (host side)", "JTAG (target side)"}, {"TCK/TMS/TDI/TDO drive", "target VREF sense", "SRST (open-drain)"}};
    }
    void fillTelemetry(Telemetry& t, std::uint64_t s) const override {
        t.channels = {{"sim_scan_count", static_cast<double>(s)}, {"sim_vref_sensed", 1.0}};
    }
};

}  // namespace

std::vector<std::unique_ptr<MockDevice>> makeMockDeviceSet() {
    std::vector<std::unique_ptr<MockDevice>> v;
    v.push_back(std::make_unique<McuMock>());
    v.push_back(std::make_unique<FpgaMock>());
    v.push_back(std::make_unique<JtagMock>());
    return v;
}

}  // namespace aether::hardware
