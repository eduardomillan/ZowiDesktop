#pragma once

#include <zowi/bluetooth_api.h>
#include <winrt/Windows.Devices.Enumeration.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>

namespace zowi {

class NativeBluetoothBackend : public BluetoothApi {
public:
    NativeBluetoothBackend();
    ~NativeBluetoothBackend() override;

    static bool hasAdapter();

    bool init() override;
    bool isAdapterAvailable() const override;
    void startDiscovery() override;
    void stopDiscovery() override;
    bool connect(const std::string &address) override;
    void disconnect() override;
    bool send(const std::string &data) override;
    bool isConnected() const override;
    std::string lastError() const override;
    void setAutoReconnect(bool enabled, int reconnectIntervalMs = 3000) override;
    void unpair(const std::string &address) override;

private:
    void receiveLoop();
    void stopReceiveLoop();
    void startReceiveLoop();
    void startReconnectThread();

    mutable std::mutex m_mutex;
    std::string m_lastError;
    std::string m_deviceAddress;
    std::string m_pendingWrite;

    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_discovering{false};
    std::atomic<bool> m_receiving{false};
    std::thread m_receiveThread;

    std::vector<std::string> m_discoveredAddrs;
    std::vector<std::string> m_discoveredNames;
    
    // Almacenar dispositivos completos para emparejamiento
    std::map<std::string, winrt::Windows::Devices::Enumeration::DeviceInformation> m_discoveredDevices;

    bool m_autoReconnect = true;
    int m_reconnectInterval = 3000;
    std::thread m_reconnectThread;
    std::atomic<bool> m_stopReconnect{false};

    struct Impl;
    std::unique_ptr<Impl> m_impl;

public:
    std::vector<std::string> listDiscoveredAddresses() const;
    std::vector<std::string> listDiscoveredNames() const;
};

} // namespace zowi
