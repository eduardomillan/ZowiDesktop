#include "native_bluetooth_backend.h"

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Rfcomm.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.SerialCommunication.h>
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
using namespace ::winrt::Windows::Devices::SerialCommunication;
using namespace ::winrt::Windows::Networking::Sockets;
using namespace ::winrt::Windows::Storage::Streams;
}

// Bring winrt aliases into file scope for use in zowi:: member definitions.
using winrt::BluetoothDevice;
using winrt::BluetoothCacheMode;
using winrt::BluetoothError;
using winrt::DeviceInformation;
using winrt::DevicePairingKinds;
using winrt::DevicePairingProtectionLevel;
using winrt::DevicePairingResultStatus;
using winrt::DeviceUnpairingResultStatus;
using winrt::InputStreamOptions;
using winrt::RfcommDeviceService;
using winrt::SerialDevice;
using winrt::StreamSocket;
using winrt::SocketQualityOfService;
using winrt::DataWriter;
using winrt::DataReader;

namespace {

constexpr winrt::guid SPP_SERVICE_UUID{
    0x00001101, 0x0000, 0x1000, {0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB}};

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
    // Formato: Bluetooth#Bluetooth<adapter_addr>-<device_addr>
    // Ejemplo: Bluetooth#Bluetooth00:1a:7d:da:71:13-c0:84:7d:aa:77:4d
    // Extraer la segunda dirección MAC (dispositivo remoto)
    std::regex re(R"(Bluetooth#Bluetooth[\dA-Fa-f:]+-([\dA-Fa-f:]{17}))");
    std::smatch m;
    if (std::regex_search(s, m, re) && m.size() >= 2)
        return m[1].str();
    return {};
}

} // namespace

namespace zowi {

struct NativeBluetoothBackend::Impl {
    // SerialDevice path (preferred for SPP)
    winrt::Windows::Devices::SerialCommunication::SerialDevice serialDevice{nullptr};
    // StreamSocket fallback (when SerialDevice unavailable)
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
    stopDiscovery();
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
    try {
        BT_LOG("Starting device discovery with DeviceWatcher...");
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_discoveredAddrs.clear();
            m_discoveredNames.clear();
            m_discoveredDevices.clear();
        }
        
        winrt::hstring selector = winrt::Windows::Devices::Bluetooth::BluetoothDevice::GetDeviceSelectorFromPairingState(false);
        
        m_watcher = winrt::DeviceInformation::CreateWatcher(selector);
        
        m_addedToken = m_watcher.Added([this](auto const& sender, auto const& deviceInfo) {
            onDeviceAdded(sender, deviceInfo);
        });
        
        m_updatedToken = m_watcher.Updated([this](auto const& sender, auto const& deviceInfoUpdate) {
            onDeviceUpdated(sender, deviceInfoUpdate);
        });
        
        m_removedToken = m_watcher.Removed([this](auto const& sender, auto const& deviceInfoUpdate) {
            onDeviceRemoved(sender, deviceInfoUpdate);
        });
        
        m_completedToken = m_watcher.EnumerationCompleted([this](auto const& sender, auto const& e) {
            onEnumerationCompleted(sender, e);
        });
        
        m_discovering = true;
        
        m_watcher.Start();
        
        BT_LOG("DeviceWatcher started successfully");
        
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
}

void NativeBluetoothBackend::stopDiscovery() {
    BT_LOG("Stopping device discovery...");
    
    m_discovering = false;
    
    if (m_watcher) {
        try {
            m_watcher.Stop();
            
            m_watcher.Added(m_addedToken);
            m_watcher.Updated(m_updatedToken);
            m_watcher.Removed(m_removedToken);
            m_watcher.EnumerationCompleted(m_completedToken);
            
            m_watcher = nullptr;
            
            BT_LOG("DeviceWatcher stopped successfully");
        } catch (...) {
            BT_LOG("Error stopping DeviceWatcher");
        }
    }
    
    if (m_onScanFinished) m_onScanFinished();
}

void NativeBluetoothBackend::onDeviceAdded(
    winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender,
    winrt::Windows::Devices::Enumeration::DeviceInformation const& deviceInfo) {
    
    try {
        auto deviceId = winrt::to_string(deviceInfo.Id());
        BT_LOG("Device added - ID: " + deviceId);
        BT_LOG("Device added - Name: " + winrt::to_string(deviceInfo.Name()));
        
        auto addr = extractBtAddress(deviceInfo.Id());
        if (addr.empty()) {
            BT_LOG("Device added with no valid address, skipping");
            return;
        }

        std::string name = winrt::to_string(deviceInfo.Name());
        if (name.empty()) name = "(unknown)";
        for (auto &c : name) {
            if (c < 0x20 || c > 0x7E) { c = '?'; }
        }

        BT_LOG("Device added: " + name + " address: " + addr + 
               " paired: " + (deviceInfo.Pairing().IsPaired() ? "true" : "false"));

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_discoveredAddrs.push_back(addr);
            m_discoveredNames.push_back(name);
            m_discoveredDevices.insert_or_assign(addr, deviceInfo);
        }

        DeviceInfo info;
        info.name = name;
        info.address = addr;
        if (m_onDevice) m_onDevice(info);
        
    } catch (...) {
        BT_LOG("Error in onDeviceAdded");
    }
}

void NativeBluetoothBackend::onDeviceUpdated(
    winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender,
    winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate) {
    
    try {
        auto addr = extractBtAddress(deviceInfoUpdate.Id());
        if (addr.empty()) return;

        BT_LOG("Device updated: " + addr);
        
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_discoveredDevices.find(addr);
        if (it != m_discoveredDevices.end()) {
            auto updatedInfo = winrt::DeviceInformation::CreateFromIdAsync(deviceInfoUpdate.Id()).get();
            it->second = updatedInfo;
        }
    } catch (...) {
        BT_LOG("Error in onDeviceUpdated");
    }
}

void NativeBluetoothBackend::onDeviceRemoved(
    winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender,
    winrt::Windows::Devices::Enumeration::DeviceInformationUpdate const& deviceInfoUpdate) {
    
    try {
        auto addr = extractBtAddress(deviceInfoUpdate.Id());
        if (addr.empty()) return;

        BT_LOG("Device removed: " + addr);
        
        std::lock_guard<std::mutex> lock(m_mutex);
        m_discoveredDevices.erase(addr);
        
        for (size_t i = 0; i < m_discoveredAddrs.size(); ++i) {
            if (m_discoveredAddrs[i] == addr) {
                m_discoveredAddrs.erase(m_discoveredAddrs.begin() + i);
                m_discoveredNames.erase(m_discoveredNames.begin() + i);
                break;
            }
        }
    } catch (...) {
        BT_LOG("Error in onDeviceRemoved");
    }
}

void NativeBluetoothBackend::onEnumerationCompleted(
    winrt::Windows::Devices::Enumeration::DeviceWatcher const& sender,
    winrt::Windows::Foundation::IInspectable const& e) {
    
    BT_LOG("Device enumeration completed");
    m_discovering = false;
    
    if (m_onScanFinished) m_onScanFinished();
}

// ── Resolve DeviceInformation by MAC address ───────────────────
winrt::Windows::Devices::Enumeration::DeviceInformation
NativeBluetoothBackend::resolveDeviceByAddress(const std::string &address)
{
    // 1) Check in-memory discovered map first
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_discoveredDevices.find(address);
        if (it != m_discoveredDevices.end()) return it->second;
    }

    // 2) Enumerate all Bluetooth devices and match by MAC
    BT_LOG("Resolving device by MAC: " + address);
    auto selector = BluetoothDevice::GetDeviceSelector();
    auto devices = DeviceInformation::FindAllAsync(selector).get();
    for (uint32_t i = 0; i < devices.Size(); ++i) {
        auto dev = devices.GetAt(i);
        if (extractBtAddress(dev.Id()) == address) {
            BT_LOG("Resolved device, ID: " + winrt::to_string(dev.Id()));
            return dev;
        }
    }
    return nullptr;
}

// ── Pair device with PIN "1234" ───────────────────────────────
bool NativeBluetoothBackend::ensurePaired(
    winrt::Windows::Devices::Enumeration::DeviceInformation &devInfo)
{
    devInfo = winrt::DeviceInformation::CreateFromIdAsync(devInfo.Id()).get();

    if (devInfo.Pairing().IsPaired()) {
        BT_LOG("Device already paired");
        return true;
    }

    BT_LOG("Device not paired, attempting auto-pair with PIN 1234...");
    auto customPairing = devInfo.Pairing().Custom();

    auto token = customPairing.PairingRequested(
        [](auto const&, auto const& args) {
            BT_LOG("Pairing requested, providing PIN 1234");
            args.Accept(L"1234");
        });

    auto result = customPairing.PairAsync(
        DevicePairingKinds::ProvidePin,
        DevicePairingProtectionLevel::None
    ).get();

    customPairing.PairingRequested(token);

    BT_LOG("Pairing result: " + std::to_string(static_cast<int>(result.Status())));

    if (result.Status() != DevicePairingResultStatus::Paired) {
        m_lastError = "Auto-pairing failed";
        BT_LOG(m_lastError);
        if (m_onError) m_onError(m_lastError);
        return false;
    }

    devInfo = winrt::DeviceInformation::CreateFromIdAsync(devInfo.Id()).get();
    BT_LOG("Pairing successful");
    return true;
}

// ── Try opening via SerialDevice (virtual COM port) ────────────
bool NativeBluetoothBackend::connectSerialDevice(const std::string &address)
{
    BT_LOG("Trying SerialDevice path for " + address);

    auto selector = winrt::Windows::Devices::SerialCommunication::SerialDevice::GetDeviceSelector();
    auto serialDevices = DeviceInformation::FindAllAsync(selector).get();
    BT_LOG("Found " + std::to_string(serialDevices.Size()) + " serial devices");

    DeviceInformation serialDevInfo{nullptr};
    std::string dashAddr = toDashAddr(address);
    std::string upperDash = dashAddr;
    for (auto &c : upperDash) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));

    for (uint32_t i = 0; i < serialDevices.Size(); ++i) {
        auto dev = serialDevices.GetAt(i);
        auto id = winrt::to_string(dev.Id());
        BT_LOG("  Serial device " + std::to_string(i) + ": " + id);

        // SerialDevice IDs look like:
        //   SWD\DEVICE\VID_0000&PID_0000\BTDEVICE_B4_9D_0B_33_8B_C6
        // or contain the MAC in various formats. Check case-insensitively.
        std::string upperId = id;
        for (auto &c : upperId) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));

        // Match MAC in dash format: B4-9D-0B-33-8B-C6
        std::string macDash = upperDash;
        if (upperId.find(macDash) != std::string::npos) {
            serialDevInfo = dev;
            BT_LOG("  -> matched by dash MAC");
            break;
        }

        // Match MAC in underscore format: B4_9D_0B_33_8B_C6
        std::string macUnderscore = upperDash;
        for (auto &c : macUnderscore) if (c == '-') c = '_';
        if (upperId.find(macUnderscore) != std::string::npos) {
            serialDevInfo = dev;
            BT_LOG("  -> matched by underscore MAC");
            break;
        }

        // Match MAC in colon format: B4:9D:0B:33:8B:C6
        std::string macColon = toColonAddr(address);
        std::string upperColon = macColon;
        for (auto &c : upperColon) c = static_cast<char>(::toupper(static_cast<unsigned char>(c)));
        if (upperId.find(upperColon) != std::string::npos) {
            serialDevInfo = dev;
            BT_LOG("  -> matched by colon MAC");
            break;
        }

        // Match raw hex MAC (no separators): B49D0B338BC6
        std::string macRaw;
        for (auto &c : upperDash) if (c != '-') macRaw += c;
        if (upperId.find(macRaw) != std::string::npos) {
            serialDevInfo = dev;
            BT_LOG("  -> matched by raw hex MAC");
            break;
        }
    }

    if (!serialDevInfo) {
        BT_LOG("No matching serial device found for this MAC");
        return false;
    }

    auto serialDevice = winrt::Windows::Devices::SerialCommunication::SerialDevice::FromIdAsync(
        serialDevInfo.Id()).get();

    if (!serialDevice) {
        BT_LOG("SerialDevice::FromIdAsync returned null (permission issue?)");
        return false;
    }

    // Configure serial parameters for HC-05
    serialDevice.BaudRate(9600);
    serialDevice.DataBits(8);
    serialDevice.Parity(winrt::Windows::Devices::SerialCommunication::SerialParity::None);
    serialDevice.StopBits(winrt::Windows::Devices::SerialCommunication::SerialStopBitCount::One);
    serialDevice.Handshake(winrt::Windows::Devices::SerialCommunication::SerialHandshake::None);

    // ReadTimeout: LoadAsync will throw after this duration if no data arrives.
    // Unlike CancellationToken, this does NOT destroy the socket/connection.
    serialDevice.ReadTimeout(std::chrono::milliseconds(500));
    serialDevice.WriteTimeout(std::chrono::seconds(5));

    BT_LOG("SerialDevice opened, baud=" + std::to_string(serialDevice.BaudRate()));

    // Close any existing connection
    closeStreams();

    m_impl->serialDevice = serialDevice;
    m_impl->writer = DataWriter(serialDevice.OutputStream());
    m_impl->reader = DataReader(serialDevice.InputStream());
    m_impl->reader.InputStreamOptions(InputStreamOptions::Partial);

    m_deviceAddress = address;
    m_connected = true;

    BT_LOG("SerialDevice connection successful, starting receive loop...");
    if (m_onConnect) m_onConnect(true);
    startReceiveLoop();
    return true;
}

// ── Fallback: connect via StreamSocket + RFCOMM ────────────────
bool NativeBluetoothBackend::connectStreamSocket(const std::string &address,
                                                  winrt::Windows::Devices::Enumeration::DeviceInformation devInfo)
{
    BT_LOG("Trying StreamSocket path for " + address);

    auto btDevice = BluetoothDevice::FromIdAsync(devInfo.Id()).get();
    auto servicesResult = btDevice.GetRfcommServicesAsync(BluetoothCacheMode::Uncached).get();

    if (servicesResult.Error() != BluetoothError::Success) {
        BT_LOG("RFCOMM service discovery failed, error: " +
               std::to_string(static_cast<int>(servicesResult.Error())));
        m_lastError = "No RFCOMM services found";
        if (m_onError) m_onError(m_lastError);
        return false;
    }

    BT_LOG("Found " + std::to_string(servicesResult.Services().Size()) + " RFCOMM services");

    RfcommDeviceService sppService{nullptr};
    auto services = servicesResult.Services();
    for (uint32_t i = 0; i < services.Size(); ++i) {
        auto service = services.GetAt(i);
        auto uuid = service.ServiceId().Uuid();

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
        m_lastError = "SPP service not found on device";
        if (m_onError) m_onError(m_lastError);
        return false;
    }

    closeStreams();

    m_impl->socket = StreamSocket();
    m_impl->socket.Control().QualityOfService(SocketQualityOfService::LowLatency);

    BT_LOG("Connecting StreamSocket to SPP service...");
    m_impl->socket.ConnectAsync(
        sppService.ConnectionHostName(),
        sppService.ConnectionServiceName()).get();

    BT_LOG("StreamSocket connected, waiting 2s for init...");
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    m_impl->writer = DataWriter(m_impl->socket.OutputStream());
    m_impl->reader = DataReader(m_impl->socket.InputStream());
    m_impl->reader.InputStreamOptions(InputStreamOptions::Partial);

    BT_LOG("Writer and Reader created");
    BT_LOG("  Local:  " + winrt::to_string(m_impl->socket.Information().LocalAddress().CanonicalName()));
    BT_LOG("  Remote: " + winrt::to_string(m_impl->socket.Information().RemoteAddress().CanonicalName()));

    m_deviceAddress = address;
    m_connected = true;

    BT_LOG("StreamSocket connection successful, starting receive loop...");
    if (m_onConnect) m_onConnect(true);
    startReceiveLoop();
    return true;
}

// ── Close existing writer/reader/socket/serialDevice ───────────
void NativeBluetoothBackend::closeStreams()
{
    try { if (m_impl->writer) { m_impl->writer.DetachStream(); m_impl->writer.Close(); m_impl->writer = nullptr; } } catch (...) {}
    try { if (m_impl->reader) { m_impl->reader.DetachStream(); m_impl->reader.Close(); m_impl->reader = nullptr; } } catch (...) {}
    try { if (m_impl->socket) { m_impl->socket.Close(); m_impl->socket = nullptr; } } catch (...) {}
    try { if (m_impl->serialDevice) { m_impl->serialDevice.Close(); m_impl->serialDevice = nullptr; } } catch (...) {}
}

// ── Main connect entry point ───────────────────────────────────
bool NativeBluetoothBackend::connect(const std::string &address) {
    try {
        BT_LOG("=== Connecting to " + address + " ===");

        // 1. Resolve DeviceInformation
        auto devInfo = resolveDeviceByAddress(address);
        if (!devInfo) {
            BT_LOG("Device not found");
            m_lastError = "Device not found. Make sure it is powered on and in range.";
            if (m_onError) m_onError(m_lastError);
            return false;
        }

        BT_LOG("Device ID: " + winrt::to_string(devInfo.Id()));
        BT_LOG("IsPaired: " + std::string(devInfo.Pairing().IsPaired() ? "true" : "false"));

        // 2. Pair if needed
        if (!devInfo.Pairing().IsPaired()) {
            if (!ensurePaired(devInfo))
                return false;
        }

        // 3. Primary path: SerialDevice (virtual COM port)
        //    After pairing, Windows creates a virtual COM port for SPP devices.
        //    SerialDevice has proper ReadTimeout that does NOT destroy the
        //    connection, unlike StreamSocket CancellationToken behavior.
        if (connectSerialDevice(address))
            return true;

        BT_LOG("SerialDevice unavailable, falling back to StreamSocket...");

        // 4. Fallback: StreamSocket + RFCOMM
        //    Known to be unreliable with HC-05/HC-06 modules but may work on
        //    some systems where the SerialDevice path isn't available.
        if (connectStreamSocket(address, devInfo))
            return true;

        // Both paths failed
        if (m_lastError.empty()) {
            m_lastError = "Could not establish SPP connection";
            if (m_onError) m_onError(m_lastError);
        }
        return false;

    } catch (const winrt::hresult_error &e) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = winrt::to_string(e.message());
        BT_LOG("Connection error: " + m_lastError);
        if (m_onError) m_onError(m_lastError);
        return false;
    } catch (...) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lastError = "Connection failed with unknown error";
        BT_LOG(m_lastError);
        if (m_onError) m_onError(m_lastError);
        return false;
    }
}

void NativeBluetoothBackend::disconnect() {
    m_stopReconnect = true;
    s_reconnectCv.notify_all();
    // Don't join the reconnect thread here — it might be blocked in connect()
    // which can't be cancelled. Let it check m_stopReconnect and exit on its own.

    stopReceiveLoop();

    m_connected = false;
    closeStreams();

    bool wasConnected = !m_deviceAddress.empty();
    m_deviceAddress.clear();

    if (wasConnected && m_onConnect)
        m_onConnect(false);
}

bool NativeBluetoothBackend::send(const std::string &data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_impl->writer || !m_connected.load()) {
        BT_LOG("Send queued (not connected): " + data);
        m_pendingWrite += data;
        return true;
    }

    try {
        BT_LOG("Sending " + std::to_string(data.size()) + " bytes: " + data);
        
        winrt::array_view<const uint8_t> view(
            reinterpret_cast<const uint8_t *>(data.data()), data.size());
        
        m_impl->writer.WriteBytes(view);
        
        auto storeOp = m_impl->writer.StoreAsync();
        storeOp.get();
        
        BT_LOG("Send successful");
        return true;
    } catch (const winrt::hresult_error &e) {
        m_lastError = "Send failed: " + winrt::to_string(e.message());
        BT_LOG(m_lastError);
        if (m_onError) m_onError(m_lastError);
        return false;
    } catch (...) {
        m_lastError = "Send failed";
        BT_LOG(m_lastError);
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
            auto devInfo = resolveDeviceByAddress(address);

            if (!devInfo || !devInfo.Pairing().IsPaired()) {
                if (m_onUnpairResult) m_onUnpairResult(true, {});
                return;
            }

            auto result = devInfo.Pairing().UnpairAsync().get();
            bool ok = (result.Status() == DeviceUnpairingResultStatus::Unpaired);
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
    BT_LOG("Receive loop started");
    
    while (m_receiving.load()) {
        try {
            if (!m_impl->reader) {
                BT_LOG("Reader is null, exiting receive loop");
                break;
            }

            auto loadOp = m_impl->reader.LoadAsync(4096);

            // Store the operation so stopReceiveLoop() can cancel it.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_loadOperation = loadOp;
            }

            auto bytesLoaded = loadOp.get();

            // Clear after successful completion.
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_loadOperation = nullptr;
            }

            if (bytesLoaded == 0) {
                // With SerialDevice this means ReadTimeout expired (no data).
                // With StreamSocket this means connection closed.
                if (m_impl->serialDevice) {
                    // SerialDevice timeout: just continue the loop.
                    // m_receiving will be checked on next iteration.
                    continue;
                }
                // StreamSocket: peer closed the connection
                BT_LOG("No bytes loaded (StreamSocket), connection closed");
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
            BT_LOG("Received " + std::to_string(bytesLoaded) + " bytes");
            if (m_onData) m_onData(str);

        } catch (winrt::hresult_canceled const &) {
            // Cancelled by stopReceiveLoop() — exit immediately.
            BT_LOG("LoadAsync cancelled, exiting receive loop");
            break;
        } catch (winrt::hresult_error const &e) {
            auto code = e.code();
            
            // SerialDevice ReadTimeout throws HRESULT 0x80070102 (ERROR_WAIT_NO) or
            // similar. Treat as "no data available" and continue.
            if (m_impl->serialDevice && m_receiving.load()) {
                // Timeout or transient error on serial device — keep reading.
                // Only log at debug level to avoid spam.
                BT_LOG("SerialDevice timeout/error (code 0x" +
                       std::to_string(static_cast<uint32_t>(code)) + "), continuing");
                continue;
            }

            BT_LOG("Receive error: " + winrt::to_string(e.message()) + " (code: 0x" + 
                   std::to_string(static_cast<uint32_t>(code)) + ")");
            
            if (!m_receiving.load()) {
                BT_LOG("Receive loop stopping");
                break;
            }
            
            if (m_receiving.load()) {
                m_receiving = false;
                m_connected = false;
                if (m_onConnect) m_onConnect(false);
                if (m_autoReconnect) startReconnectThread();
            }
            break;
        } catch (...) {
            if (m_impl->serialDevice && m_receiving.load()) {
                // Transient error on serial device — keep trying
                continue;
            }
            BT_LOG("Unknown receive error");
            break;
        }
    }
    BT_LOG("Receive loop ended");
}

void NativeBluetoothBackend::stopReceiveLoop() {
    BT_LOG("Stopping receive loop...");
    m_receiving = false;

    // Cancel any pending LoadAsync operation.
    // IAsyncOperation::Cancel() is thread-safe and causes LoadAsync().get()
    // to throw hresult_canceled immediately, regardless of ReadTimeout.
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_loadOperation) {
            m_loadOperation.Cancel();
            m_loadOperation = nullptr;
        }
    }

    // For StreamSocket: also close the socket as a safety net.
    try {
        if (m_impl->socket) {
            m_impl->socket.Close();
            m_impl->socket = nullptr;
        }
    } catch (...) {}

    if (m_receiveThread.joinable()) {
        BT_LOG("Waiting for receive thread to finish...");
        m_receiveThread.join();
        BT_LOG("Receive thread finished");
    }

    // Reader/writer cleanup is handled by closeStreams() in disconnect().
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
