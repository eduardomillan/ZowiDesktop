#include "native_bluetooth_backend.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Networking.Sockets.h>
#include <winrt/Windows.Storage.Streams.h>

#include <memory>
#include <regex>
#include <condition_variable>
#include <iostream>
#include <windows.h>

#pragma comment(lib, "runtimeobject.lib")

#define BT_LOG(msg) do { std::string log_msg = "[bt_native] " + std::string(msg); OutputDebugStringA((log_msg + "\n").c_str()); std::cout << log_msg << std::endl; } while(0)

namespace winrt {
using namespace ::winrt::Windows::Foundation;
using namespace ::winrt::Windows::Devices::Bluetooth;
using namespace ::winrt::Windows::Devices::Bluetooth::Rfcomm;
using namespace ::winrt::Windows::Devices::Enumeration;
using namespace ::winrt::Windows::Networking::Sockets;
using namespace ::winrt::Windows::Storage::Streams;
}

namespace {

constexpr winrt::guid SPP_SERVICE_UUID{
    0x00001101, 0x0000, 0x1000, {0x80, 0x00, 0x00, 0x80, 0x5F, 0xB3, 0x4F, 0x9B}};

std::string toColonAddr(const std::string &dashes) {
    std::string r = dashes;
    for (auto &c : r) if (c == '-') c = ':';
    return r;
}

std::string toDashAddr(const std::string &colons) {
    std::string r = colons;
    for (auto &c : r) if (c == ':') c = '-';
    return r;
}

std::string extractBtAddress(const winrt::hstring &deviceId) {
    auto s = winrt::to_string(deviceId);
    std::regex re(R"(([\dA-Fa-f]{2})-([\dA-Fa-f]{2})-([\dA-Fa-f]{2})-([\dA-Fa-f]{2})-([\dA-Fa-f]{2})-([\dA-Fa-f]{2}))");
    std::smatch m;
    if (std::regex_search(s, m, re) && m.size() >= 7)
        return m[1].str() + ":" + m[2].str() + ":" + m[3].str() + ":" +
               m[4].str() + ":" + m[5].str() + ":" + m[6].str();
    return {};
}

} // namespace

namespace zowi {

struct NativeBluetoothBackend::Impl {
    winrt::StreamSocket socket{nullptr};
    winrt::DataWriter writer{nullptr};
    winrt::DataReader reader{nullptr};
};

static std::mutex s_reconnectCvMutex;
static std::condition_variable s_reconnectCv;

NativeBluetoothBackend::NativeBluetoothBackend()
    : m_impl(std::make_unique<Impl>()) {}

NativeBluetoothBackend::~NativeBluetoothBackend() {
    m_stopReconnect = true;
    s_reconnectCv.notify_all();
    if (m_reconnectThread.joinable())
        m_reconnectThread.join();
    stopReceiveLoop();
    disconnect();
}

bool NativeBluetoothBackend::init() { return true; }

bool NativeBluetoothBackend::hasAdapter() {
    try {
        auto adapters = winrt::DeviceInformation::FindAllAsync(
            winrt::Windows::Devices::Bluetooth::BluetoothAdapter::GetDeviceSelector()).get();
        return adapters.Size() > 0;
    } catch (...) {
        return false;
    }
}

bool NativeBluetoothBackend::isAdapterAvailable() const {
    return hasAdapter();
}

void NativeBluetoothBackend::startDiscovery() {
    std::thread([this]() {
        try {
            BT_LOG("Starting device discovery...");
            
            // Selector para dispositivos Bluetooth Classic (incluye emparejados y no emparejados)
            // GUID e0cbf06c-cd8b-4647-bb8a-263b43f0f974 = Bluetooth Classic
            winrt::hstring selector = L"System.Devices.Aep.ProtocolId:=\"{e0cbf06c-cd8b-4647-bb8a-263b43f0f974}\"";
            
            auto devices = winrt::DeviceInformation::FindAllAsync(selector).get();

            BT_LOG("Found " + std::to_string(devices.Size()) + " Bluetooth devices");

            for (uint32_t i = 0; i < devices.Size(); ++i) {
                if (!m_discovering.load()) break;

                auto dev = devices.GetAt(i);
                auto addr = extractBtAddress(dev.Id());
                if (addr.empty()) {
                    BT_LOG("Device " + std::to_string(i) + " has no valid address, skipping");
                    continue;
                }

                std::string name = winrt::to_string(dev.Name());
                if (name.empty()) name = "(unknown)";
                for (auto &c : name) {
                    if (c < 0x20 || c > 0x7E) { c = '?'; }
                }

                BT_LOG("Discovered device: " + name + " address: " + addr + 
                       " paired: " + (dev.Pairing().IsPaired() ? "true" : "false"));

                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    m_discoveredAddrs.push_back(addr);
                    m_discoveredNames.push_back(name);
                    // Almacenar el dispositivo completo para emparejamiento
                    m_discoveredDevices.insert_or_assign(addr, dev);
                }

                DeviceInfo info;
                info.name = name;
                info.address = addr;
                if (m_onDevice) m_onDevice(info);
            }
        } catch (const winrt::hresult_error &e) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastError = winrt::to_string(e.message());
            BT_LOG("Discovery error: " + m_lastError);
            if (m_onError) m_onError(m_lastError);
        } catch (...) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastError = "Discovery failed";
            BT_LOG("Discovery failed with unknown error");
            if (m_onError) m_onError(m_lastError);
        }

        m_discovering = false;
        BT_LOG("Discovery finished");
        if (m_onScanFinished) m_onScanFinished();
    }).detach();

    m_discovering = true;
}

void NativeBluetoothBackend::stopDiscovery() {
    m_discovering = false;
}

bool NativeBluetoothBackend::connect(const std::string &address) {
    try {
        BT_LOG("Connecting to " + address);
        
        // Buscar el dispositivo en el mapa de dispositivos descubiertos
        winrt::Windows::Devices::Enumeration::DeviceInformation devInfo{nullptr};
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_discoveredDevices.find(address);
            if (it == m_discoveredDevices.end()) {
                BT_LOG("Device not found in discovered devices map");
                m_lastError = "Device not found in scan results. Please scan first.";
                if (m_onError) m_onError(m_lastError);
                return false;
            }
            devInfo = it->second;
        }

        BT_LOG("Found device in map, checking pairing status...");
        BT_LOG("Device ID: " + winrt::to_string(devInfo.Id()));
        BT_LOG("IsPaired: " + std::string(devInfo.Pairing().IsPaired() ? "true" : "false"));
        BT_LOG("CanPair: " + std::string(devInfo.Pairing().CanPair() ? "true" : "false"));

        // Auto-pair if not already paired (similar to BlueZ agent on Linux)
        if (!devInfo.Pairing().IsPaired()) {
            BT_LOG("Device not paired, attempting auto-pair...");
            
            if (!devInfo.Pairing().CanPair()) {
                BT_LOG("Device cannot be paired");
                std::lock_guard<std::mutex> lock(m_mutex);
                m_lastError = "Device cannot be paired";
                if (m_onError) m_onError(m_lastError);
                return false;
            }

            // Use custom pairing with automatic PIN "1234" for HC-05
            auto customPairing = devInfo.Pairing().Custom();
            
            // Register handler to provide PIN when requested
            auto pairingRequestedToken = customPairing.PairingRequested(
                [](winrt::Windows::Devices::Enumeration::DeviceInformationCustomPairing const& sender,
                   winrt::Windows::Devices::Enumeration::DevicePairingRequestedEventArgs const& args) {
                    BT_LOG("Pairing requested, providing PIN 1234");
                    // Auto-accept with PIN "1234" for HC-05 modules
                    args.Accept(L"1234");
                });

            BT_LOG("Starting PairAsync with DevicePairingProtectionLevel::None...");
            
            // Perform pairing with no protection (HC-05 uses legacy pairing)
            auto pairResult = customPairing.PairAsync(
                winrt::Windows::Devices::Enumeration::DevicePairingKinds::ProvidePin,
                winrt::Windows::Devices::Enumeration::DevicePairingProtectionLevel::None
            ).get();

            // Remove the handler
            customPairing.PairingRequested(pairingRequestedToken);

            BT_LOG("Pairing result status: " + std::to_string(static_cast<int>(pairResult.Status())));

            if (pairResult.Status() != winrt::Windows::Devices::Enumeration::DevicePairingResultStatus::Paired) {
                BT_LOG("Auto-pairing failed with status: " + std::to_string(static_cast<int>(pairResult.Status())));
                std::lock_guard<std::mutex> lock(m_mutex);
                m_lastError = "Auto-pairing failed";
                if (m_onError) m_onError(m_lastError);
                return false;
            }

            BT_LOG("Pairing successful, refreshing device info...");
            
            // Refresh device info after pairing
            devInfo = winrt::DeviceInformation::CreateFromIdAsync(devInfo.Id()).get();
        }

        BT_LOG("Getting Bluetooth device from ID...");
        auto btDevice = winrt::Windows::Devices::Bluetooth::BluetoothDevice::FromIdAsync(devInfo.Id()).get();
        
        BT_LOG("Getting RFCOMM services...");
        auto servicesResult = btDevice.GetRfcommServicesAsync(
            winrt::Windows::Devices::Bluetooth::BluetoothCacheMode::Uncached).get();

        if (servicesResult.Error() != winrt::Windows::Devices::Bluetooth::BluetoothError::Success) {
            BT_LOG("No RFCOMM services found, error: " + std::to_string(static_cast<int>(servicesResult.Error())));
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastError = "No RFCOMM services found";
            if (m_onError) m_onError(m_lastError);
            return false;
        }

        BT_LOG("Found " + std::to_string(servicesResult.Services().Size()) + " RFCOMM services");

        winrt::Windows::Devices::Bluetooth::Rfcomm::RfcommDeviceService sppService{nullptr};
        auto services = servicesResult.Services();
        for (uint32_t i = 0; i < services.Size(); ++i) {
            auto service = services.GetAt(i);
            auto uuid = service.ServiceId().Uuid();
            
            // Format UUID as string for logging
            char uuid_str[64];
            snprintf(uuid_str, sizeof(uuid_str), 
                "{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                uuid.Data1, uuid.Data2, uuid.Data3,
                uuid.Data4[0], uuid.Data4[1], uuid.Data4[2], uuid.Data4[3],
                uuid.Data4[4], uuid.Data4[5], uuid.Data4[6], uuid.Data4[7]);
            
            BT_LOG("Service " + std::to_string(i) + " UUID: " + uuid_str);
            
            if (service.ServiceId().Uuid() == SPP_SERVICE_UUID) {
                sppService = service;
                BT_LOG("Found SPP service");
                break;
            }
        }

        if (!sppService) {
            BT_LOG("SPP service not found on device");
            std::lock_guard<std::mutex> lock(m_mutex);
            m_lastError = "SPP service not found on device";
            if (m_onError) m_onError(m_lastError);
            return false;
        }

        BT_LOG("Closing existing connections...");
        try { if (m_impl->writer) { m_impl->writer.Close(); m_impl->writer = nullptr; } } catch (...) {}
        try { if (m_impl->reader) { m_impl->reader.Close(); m_impl->reader = nullptr; } } catch (...) {}
        try { if (m_impl->socket) { m_impl->socket.Close(); m_impl->socket = nullptr; } } catch (...) {}

        BT_LOG("Creating new StreamSocket...");
        m_impl->socket = winrt::Windows::Networking::Sockets::StreamSocket();
        m_impl->socket.Control().QualityOfService(
            winrt::Windows::Networking::Sockets::SocketQualityOfService::LowLatency);

        BT_LOG("Connecting to SPP service...");
        m_impl->socket.ConnectAsync(
            sppService.ConnectionHostName(),
            sppService.ConnectionServiceName()).get();

        BT_LOG("Creating DataWriter and DataReader...");
        m_impl->writer = winrt::Windows::Storage::Streams::DataWriter(m_impl->socket.OutputStream());
        m_impl->reader = winrt::Windows::Storage::Streams::DataReader(m_impl->socket.InputStream());

        m_deviceAddress = address;
        m_connected = true;

        BT_LOG("Connection successful, starting receive loop...");
        if (m_onConnect) m_onConnect(true);

        startReceiveLoop();
        return true;

    } catch (const winrt::hresult_error &e) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = winrt::to_string(e.message());
        BT_LOG("Connection error: " + m_lastError);
        if (m_onError) m_onError(m_lastError);
        return false;
    } catch (...) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = "Connection failed";
        BT_LOG("Connection failed with unknown error");
        if (m_onError) m_onError(m_lastError);
        return false;
    }
}

void NativeBluetoothBackend::disconnect() {
    m_stopReconnect = true;
    s_reconnectCv.notify_all();
    if (m_reconnectThread.joinable())
        m_reconnectThread.join();

    stopReceiveLoop();

    m_connected = false;

    try {
        if (m_impl->writer) {
            m_impl->writer.DetachStream();
            m_impl->writer.Close();
            m_impl->writer = nullptr;
        }
    } catch (...) {}

    try {
        if (m_impl->reader) {
            m_impl->reader.DetachStream();
            m_impl->reader.Close();
            m_impl->reader = nullptr;
        }
    } catch (...) {}

    try {
        if (m_impl->socket) {
            m_impl->socket.Close();
            m_impl->socket = nullptr;
        }
    } catch (...) {}

    bool wasConnected = !m_deviceAddress.empty();
    m_deviceAddress.clear();

    if (wasConnected && m_onConnect)
        m_onConnect(false);
}

bool NativeBluetoothBackend::send(const std::string &data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_impl->writer || !m_connected.load()) {
        m_pendingWrite += data;
        return true;
    }

    try {
        winrt::array_view<const uint8_t> view(
            reinterpret_cast<const uint8_t *>(data.data()), data.size());
        m_impl->writer.WriteBytes(view);
        m_impl->writer.StoreAsync().get();
        return true;
    } catch (...) {
        m_lastError = "Send failed";
        if (m_onError) m_onError(m_lastError);
        return false;
    }
}

bool NativeBluetoothBackend::isConnected() const {
    return m_connected.load();
}

std::string NativeBluetoothBackend::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

void NativeBluetoothBackend::setAutoReconnect(bool enabled, int reconnectIntervalMs) {
    m_autoReconnect = enabled;
    m_reconnectInterval = reconnectIntervalMs;
    if (!enabled) {
        m_stopReconnect = true;
        s_reconnectCv.notify_all();
    }
}

void NativeBluetoothBackend::unpair(const std::string &address) {
    std::thread([this, address]() {
        try {
            std::string dashed = toDashAddr(address);
            std::string deviceId = "Bluetooth#" + dashed + "#Device";

            auto devInfo = winrt::DeviceInformation::CreateFromIdAsync(
                winrt::to_hstring(deviceId)).get();

            if (!devInfo.Pairing().IsPaired()) {
                if (m_onUnpairResult) m_onUnpairResult(true, {});
                return;
            }

            auto result = devInfo.Pairing().UnpairAsync().get();
            bool ok = (result.Status() == winrt::DeviceUnpairingResultStatus::Unpaired);
            if (m_onUnpairResult)
                m_onUnpairResult(ok, ok ? "" : "Unpair failed");
        } catch (const winrt::hresult_error &e) {
            if (m_onUnpairResult)
                m_onUnpairResult(false, winrt::to_string(e.message()));
        } catch (...) {
            if (m_onUnpairResult)
                m_onUnpairResult(false, "Unpair failed");
        }
    }).detach();
}

void NativeBluetoothBackend::receiveLoop() {
    while (m_receiving.load()) {
        try {
            if (!m_impl->reader) break;

            auto loadOp = m_impl->reader.LoadAsync(4096);
            auto bytesLoaded = loadOp.get();

            if (bytesLoaded == 0) {
                if (m_receiving.load()) {
                    m_receiving = false;
                    m_connected = false;
                    if (m_onConnect) m_onConnect(false);
                    if (m_autoReconnect) startReconnectThread();
                }
                break;
            }

            std::vector<uint8_t> buffer(bytesLoaded);
            m_impl->reader.ReadBytes(buffer);
            std::string str(reinterpret_cast<const char *>(buffer.data()), buffer.size());
            if (m_onData) m_onData(str);

        } catch (winrt::hresult_error const &) {
            if (m_receiving.load()) {
                m_receiving = false;
                m_connected = false;
                if (m_onConnect) m_onConnect(false);
                if (m_autoReconnect) startReconnectThread();
            }
            break;
        } catch (...) {
            break;
        }
    }
}

void NativeBluetoothBackend::stopReceiveLoop() {
    m_receiving = false;

    try {
        if (m_impl->reader)
            m_impl->reader.DetachStream();
    } catch (...) {}

    if (m_receiveThread.joinable())
        m_receiveThread.join();
}

void NativeBluetoothBackend::startReceiveLoop() {
    m_receiving = true;
    m_receiveThread = std::thread(&NativeBluetoothBackend::receiveLoop, this);
}

void NativeBluetoothBackend::startReconnectThread() {
    if (!m_autoReconnect || m_deviceAddress.empty()) return;

    m_stopReconnect = false;
    m_reconnectThread = std::thread([this]() {
        while (!m_stopReconnect.load()) {
            {
                std::unique_lock<std::mutex> lock(s_reconnectCvMutex);
                s_reconnectCv.wait_for(lock,
                    std::chrono::milliseconds(m_reconnectInterval),
                    [this] { return m_stopReconnect.load(); });
            }
            if (m_stopReconnect.load()) break;
            if (m_connected.load()) break;
            if (m_deviceAddress.empty()) break;

            std::string addr = m_deviceAddress;
            if (!addr.empty()) {
                disconnect();
                m_deviceAddress = addr;
                connect(addr);
            }
        }
    });
}

std::vector<std::string> NativeBluetoothBackend::listDiscoveredAddresses() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_discoveredAddrs;
}

std::vector<std::string> NativeBluetoothBackend::listDiscoveredNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_discoveredNames;
}

} // namespace zowi
