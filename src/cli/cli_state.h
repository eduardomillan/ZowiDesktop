#ifndef ZOWI_CLI_STATE_H
#define ZOWI_CLI_STATE_H

#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace zowi_cli {

// ── Shared global connection / robot state ───────────────────
// These globals are shared between the message parser (cli_state.cpp),
// the firmware/backend helpers (cli_util.cpp) and the command handlers
// (cli_commands.cpp). Kept here so the state lives in a single place.
extern std::mutex g_mtx;
extern std::condition_variable g_cv;
extern std::string g_robotName;
extern std::string g_appId;
extern float g_battery;
extern bool g_connected;
extern bool g_connectedOnce;
extern bool g_dataReceived;
extern bool g_ack;        // software ack (&&A)
extern bool g_finalAck;   // final ack (&&F), after EEPROM write
extern std::string g_dataBuffer;
// When true, incoming bytes are raw bootloader traffic, not the &&/N-U-B protocol.
extern bool g_uploadMode;
extern std::string g_stkBuffer;
extern std::atomic<bool> g_quit;
extern int g_stdinFd;

// ── Constants ────────────────────────────────────────────────
extern const float kLowBatteryThreshold;
extern const char *const kFactoryFirmwarePath;
extern const char *const kAlarmFirmwarePath;
extern const char *const kAdivinawiFirmwarePath;
extern const int kDiscoveryTimeoutMs;

void resetRobotState();

std::string trimRobotMessage(const std::string &msg);

// Parses a single already-delimited robot message (a &&-token or a legacy
// line). Updates the shared global state. Caller must NOT hold g_mtx.
void parseRobotMessage(const std::string &msg);

// Incoming-data callback for the Bluetooth backends. Buffers bytes and pulls
// out complete protocol messages, forwarding each to parseRobotMessage().
void onDataReceived(const std::string &data);

} // namespace zowi_cli

#endif // ZOWI_CLI_STATE_H
