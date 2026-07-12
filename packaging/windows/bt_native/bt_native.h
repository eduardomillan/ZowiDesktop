#ifndef BT_NATIVE_H
#define BT_NATIVE_H

#ifdef BT_NATIVE_EXPORTS
#define BT_API __declspec(dllexport)
#else
#define BT_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Device info structure
typedef struct {
    char address[18];   // "XX:XX:XX:XX:XX:XX"
    char name[64];
    int rssi;
} bt_device_t;

// Callback for device discovery
typedef void (*bt_discovery_callback)(const bt_device_t *device, void *user_data);

// Callback for data received
typedef void (*bt_data_callback)(const char *data, int length, void *user_data);

// Callback for connection state changes
typedef void (*bt_connection_callback)(int connected, void *user_data);

// Initialize the Bluetooth module
// Returns 0 on success, -1 on error
BT_API int bt_init(void);

// Start device discovery
// Calls callback for each device found
// Returns 0 on success, -1 on error
BT_API int bt_start_discovery(bt_discovery_callback callback, void *user_data);

// Stop device discovery
BT_API void bt_stop_discovery(void);

// Connect to a device by address ("XX:XX:XX:XX:XX:XX")
// Calls connection_callback when state changes
// Returns 0 on success, -1 on error
BT_API int bt_connect(const char *address, bt_connection_callback callback, void *user_data);

// Send data over the connection
// Returns number of bytes sent, -1 on error
BT_API int bt_send(const char *data, int length);

// Start receiving data
// Calls data_callback when data arrives
// Returns 0 on success, -1 on error
BT_API int bt_start_receive(bt_data_callback callback, void *user_data);

// Disconnect from the current device
BT_API void bt_disconnect(void);

// Cleanup and release resources
BT_API void bt_cleanup(void);

// Get the last error message
BT_API const char *bt_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // BT_NATIVE_H
