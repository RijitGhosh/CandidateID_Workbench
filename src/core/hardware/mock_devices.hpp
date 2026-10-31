// Aether core: SIMULATED hardware adapters. Nothing here talks to real hardware.
// All telemetry is deterministic synthetic data and is labelled "SIMULATED".
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace aether::hardware {

enum class DeviceKind { Microcontroller, Fpga, JtagAdapter };
enum class DeviceState { Undiscovered, Disconnected, Connected, Faulted };
enum class ErrorCode { None, NotDiscovered, NotConnected, AlreadyConnected, Timeout, DeviceFaulted };

const char* toString(DeviceKind k);
const char* toString(DeviceState s);
const char* toString(ErrorCode c);

struct Status {
    ErrorCode code = ErrorCode::None;
    std::string message;
    bool ok() const { return code == ErrorCode::None; }
};

struct DeviceInfo {
    DeviceKind kind;
    std::string id;           // e.g. "mock-mcu-0"
    std::string modelsBoard;  // the documented board this mock stands in for (see docs/embedded_design.md)
    bool simulated = true;    // always true in this build
};

struct Capabilities {
    std::vector<std::string> interfaces;  // interface names only; electrical data lives in the docs
    std::vector<std::string> features;
};

struct Telemetry {
    std::string deviceId;
    std::uint64_t sequence = 0;
    std::vector<std::pair<std::string, double>> channels;
    bool simulated = true;
    std::string banner = "SIMULATED - no physical hardware involved";
};

class MockDevice {
public:
    MockDevice(DeviceKind kind, std::string id, std::string modelsBoard);
    virtual ~MockDevice() = default;

    const DeviceInfo& info() const { return info_; }
    DeviceState state() const { return state_; }

    Status discover();                              // Undiscovered -> Disconnected
    Status capabilities(Capabilities& out) const;   // requires discovery
    Status connect();                               // Disconnected -> Connected
    Status disconnect();                            // Connected/Faulted -> Disconnected
    Status readTelemetry(Telemetry& out);           // one message; may time out if a fault was armed
    void armTimeoutFault() { timeoutArmed_ = true; }  // next readTelemetry() times out
    Status recover();                               // Faulted -> Connected (clears fault)

protected:
    virtual Capabilities makeCapabilities() const = 0;
    virtual void fillTelemetry(Telemetry& t, std::uint64_t seq) const = 0;

private:
    DeviceInfo info_;
    DeviceState state_ = DeviceState::Undiscovered;
    bool timeoutArmed_ = false;
    std::uint64_t seq_ = 0;
};

// Convenience factory: one microcontroller, one FPGA, one JTAG adapter.
std::vector<std::unique_ptr<MockDevice>> makeMockDeviceSet();

}  // namespace aether::hardware
