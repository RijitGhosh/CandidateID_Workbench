#include <gtest/gtest.h>

#include "core/hardware/mock_devices.hpp"

using namespace aether::hardware;

TEST(Hardware, SetContainsOneOfEachKindAndAllAreSimulated) {
    const auto devs = makeMockDeviceSet();
    ASSERT_EQ(devs.size(), 3u);
    EXPECT_EQ(devs[0]->info().kind, DeviceKind::Microcontroller);
    EXPECT_EQ(devs[1]->info().kind, DeviceKind::Fpga);
    EXPECT_EQ(devs[2]->info().kind, DeviceKind::JtagAdapter);
    for (const auto& d : devs) EXPECT_TRUE(d->info().simulated) << d->info().id;
}

TEST(Hardware, LifecycleDiscoverCapabilitiesConnectTelemetryDisconnect) {
    for (auto& d : makeMockDeviceSet()) {
        SCOPED_TRACE(d->info().id);
        EXPECT_EQ(d->state(), DeviceState::Undiscovered);
        Capabilities caps;
        EXPECT_EQ(d->capabilities(caps).code, ErrorCode::NotDiscovered) << "capabilities need discovery";
        EXPECT_EQ(d->connect().code, ErrorCode::NotDiscovered);
        ASSERT_TRUE(d->discover().ok());
        EXPECT_EQ(d->state(), DeviceState::Disconnected);
        ASSERT_TRUE(d->capabilities(caps).ok());
        EXPECT_FALSE(caps.interfaces.empty());
        Telemetry t;
        EXPECT_EQ(d->readTelemetry(t).code, ErrorCode::NotConnected);
        ASSERT_TRUE(d->connect().ok());
        EXPECT_EQ(d->connect().code, ErrorCode::AlreadyConnected);
        ASSERT_TRUE(d->readTelemetry(t).ok());
        EXPECT_EQ(t.deviceId, d->info().id);
        EXPECT_TRUE(t.simulated);
        EXPECT_NE(t.banner.find("SIMULATED"), std::string::npos);
        EXPECT_FALSE(t.channels.empty());
        ASSERT_TRUE(d->disconnect().ok());
        EXPECT_EQ(d->state(), DeviceState::Disconnected);
        EXPECT_EQ(d->disconnect().code, ErrorCode::NotConnected);
    }
}

TEST(Hardware, TelemetrySequenceIncrements) {
    auto d = std::move(makeMockDeviceSet()[0]);
    d->discover();
    d->connect();
    Telemetry a, b;
    ASSERT_TRUE(d->readTelemetry(a).ok());
    ASSERT_TRUE(d->readTelemetry(b).ok());
    EXPECT_EQ(b.sequence, a.sequence + 1);
}

TEST(Hardware, TimeoutFaultIsRecoverableForEveryDevice) {
    for (auto& d : makeMockDeviceSet()) {
        SCOPED_TRACE(d->info().id);
        d->discover();
        d->connect();
        Telemetry t;
        ASSERT_TRUE(d->readTelemetry(t).ok());
        d->armTimeoutFault();
        const Status fail = d->readTelemetry(t);
        EXPECT_EQ(fail.code, ErrorCode::Timeout);
        EXPECT_FALSE(fail.message.empty());
        EXPECT_EQ(d->state(), DeviceState::Faulted);
        EXPECT_EQ(d->readTelemetry(t).code, ErrorCode::DeviceFaulted) << "stays faulted until recovered";
        EXPECT_EQ(d->connect().code, ErrorCode::DeviceFaulted);
        ASSERT_TRUE(d->recover().ok());
        EXPECT_EQ(d->state(), DeviceState::Connected);
        EXPECT_TRUE(d->readTelemetry(t).ok()) << "telemetry must work again after recovery";
    }
}

TEST(Hardware, RecoverWithoutFaultIsAnError) {
    auto d = std::move(makeMockDeviceSet()[1]);
    d->discover();
    d->connect();
    EXPECT_FALSE(d->recover().ok());
}

TEST(Hardware, DisconnectClearsPendingFault) {
    auto d = std::move(makeMockDeviceSet()[2]);
    d->discover();
    d->connect();
    d->armTimeoutFault();
    d->disconnect();
    d->connect();
    Telemetry t;
    EXPECT_TRUE(d->readTelemetry(t).ok());
}
