#pragma once

#include <string>
#include <functional>
#include <memory>
#include <zowi/device_info.h>

namespace zowi {

class BluetoothApi {
public:
    virtual ~BluetoothApi() = default;

    using DeviceCallback    = std::function<void(const DeviceInfo &)>;
    using DataCallback      = std::function<void(const std::string &)>;
    using ConnectCallback   = std::function<void(bool connected)>;
    using ErrorCallback     = std::function<void(const std::string &)>;

    virtual bool init() = 0;
    virtual void startDiscovery() = 0;
    virtual void stopDiscovery() = 0;
    virtual bool connect(const std::string &address) = 0;
    virtual void disconnect() = 0;
    virtual bool send(const std::string &data) = 0;
    virtual bool isConnected() const = 0;
    virtual std::string lastError() const = 0;

    void onDeviceFound(DeviceCallback cb)       { m_onDevice = std::move(cb); }
    void onDataReceived(DataCallback cb)        { m_onData = std::move(cb); }
    void onConnectionChanged(ConnectCallback cb){ m_onConnect = std::move(cb); }
    void onError(ErrorCallback cb)              { m_onError = std::move(cb); }

protected:
    DeviceCallback   m_onDevice;
    DataCallback     m_onData;
    ConnectCallback  m_onConnect;
    ErrorCallback    m_onError;
};

} // namespace zowi
