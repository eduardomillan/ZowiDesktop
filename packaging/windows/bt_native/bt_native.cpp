#include "bt_native.h"
#include <string>
#include <thread>
#include <atomic>

// ============================================================================
// STUB IMPLEMENTATION
// Replace this with real WinRT Bluetooth code on Windows.
// See: https://learn.microsoft.com/en-us/windows/uwp/devices-sensors/bluetooth
// ============================================================================

static std::string s_last_error;
static std::atomic<bool> s_initialized{false};
static std::atomic<bool> s_discovering{false};
static std::thread s_discovery_thread;

static void set_error(const char *msg) {
    s_last_error = msg ? msg : "unknown error";
}

// ---- Public API -----------------------------------------------------------

extern "C" {

BT_API int bt_init(void) {
    // TODO: Initialize WinRT Bluetooth adapter
    // Windows::Devices::Bluetooth::BluetoothAdapter::GetDefaultAsync()
    s_initialized = true;
    return 0;
}

BT_API int bt_start_discovery(bt_discovery_callback callback, void *user_data) {
    if (!s_initialized) {
        set_error("not initialized");
        return -1;
    }
    if (s_discovering) {
        set_error("already discovering");
        return -1;
    }

    s_discovering = true;

    // TODO: Replace with real WinRT discovery
    // Windows::Devices::Bluetooth::BluetoothDevice::FindAllAsync()
    //
    // For now, run a dummy discovery that finds nothing.
    s_discovery_thread = std::thread([callback, user_data]() {
        // Placeholder: no devices found
        s_discovering = false;
    });
    s_discovery_thread.detach();

    return 0;
}

BT_API void bt_stop_discovery(void) {
    s_discovering = false;
    // TODO: Cancel WinRT discovery
}

BT_API int bt_connect(const char *address, bt_connection_callback callback, void *user_data) {
    if (!s_initialized) {
        set_error("not initialized");
        return -1;
    }
    if (!address || address[0] == '\0') {
        set_error("invalid address");
        return -1;
    }

    // TODO: Replace with real WinRT SPP connection
    // Windows::Devices::Bluetooth::BluetoothDevice::FromHostNameAsync()
    // -> GetRfcommServicesAsync()
    // -> OpenAsync(RfcommServiceAccessMode::ReadWrite)

    // Placeholder: report disconnected
    if (callback) callback(0, user_data);

    return 0;
}

BT_API int bt_send(const char *data, int length) {
    if (!s_initialized) {
        set_error("not initialized");
        return -1;
    }
    if (!data || length <= 0) {
        set_error("invalid data");
        return -1;
    }

    // TODO: Replace with real WinRT socket write
    // m_writer->WriteBytes(...)

    return length; // pretend all bytes sent
}

BT_API int bt_start_receive(bt_data_callback callback, void *user_data) {
    if (!s_initialized) {
        set_error("not initialized");
        return -1;
    }

    // TODO: Replace with real WinRT socket read loop
    // m_reader->ReadAsync()

    return 0;
}

BT_API void bt_disconnect(void) {
    // TODO: Close WinRT socket
    s_discovering = false;
}

BT_API void bt_cleanup(void) {
    bt_disconnect();
    s_initialized = false;
    // TODO: Release WinRT resources
}

BT_API const char *bt_last_error(void) {
    return s_last_error.c_str();
}

} // extern "C"
