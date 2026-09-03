#include "cli_state.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <QDebug>
#include <QString>
#include <zowi/message_parser.h>
#include <zowi/protocol.h>
#include <zowi/config_store.h>
#ifdef _WIN32
#include <io.h>
#endif

namespace zowi_cli {

std::mutex g_mtx;
std::condition_variable g_cv;
std::string g_robotName;
std::string g_appId;
float g_battery = -1.0f;
bool g_connected = false;
bool g_connectedOnce = false;
bool g_dataReceived = false;
bool g_ack = false;        // software ack (&&A)
bool g_finalAck = false;   // final ack (&&F), after EEPROM write
int g_finalAckCount = 0;   // &&F counter (one per completed gait cycle)
std::chrono::steady_clock::time_point g_lastRx = std::chrono::steady_clock::now();
bool g_uploadMode = false;
std::string g_stkBuffer;
std::atomic<bool> g_quit{false};
#ifdef _WIN32
int g_stdinFd = _fileno(stdin);
#else
int g_stdinFd = STDIN_FILENO;
#endif
bool g_debugLog = false;

// Shared protocol frame reassembler (&&...%% frames + legacy lines), guarded
// by g_mtx like every other piece of global state.
static zowi::MessageParser s_parser;

const float kLowBatteryThreshold = 50.0f;
const char *const kFactoryFirmwarePath = "src/firmware/ZOWI_BASE_v2.hex";
const char *const kAlarmFirmwarePath = "src/firmware/ZOWI_Alarm_v2.hex";
const char *const kAdivinawiFirmwarePath = "src/firmware/ZOWI_Adivinawi_v2.hex";
const int kDiscoveryTimeoutMs = 6000;

void resetRobotState()
{
    loadLogLevel();
    std::lock_guard<std::mutex> lock(g_mtx);
    s_parser.reset();
    g_robotName.clear();
    g_appId.clear();
    g_battery = -1.0f;
    g_connected = false;
    g_connectedOnce = false;
    g_dataReceived = false;
    g_ack = false;
    g_finalAck = false;
    g_finalAckCount = 0;
}

void loadLogLevel()
{
    zowi::ConfigStore config("src/config.json");
    g_debugLog = (config.get("log_level") == "debug");
}

static void cliMessageHandler(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    Q_UNUSED(ctx);
    switch (type) {
    case QtDebugMsg:
        if (!g_debugLog) return;
        fprintf(stderr, "[DEBUG] %s\n", msg.toUtf8().constData());
        break;
    case QtWarningMsg:
        fprintf(stderr, "[WARN] %s\n", msg.toUtf8().constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[ERROR] %s\n", msg.toUtf8().constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "[FATAL] %s\n", msg.toUtf8().constData());
        abort();
        break;
    default:
        fprintf(stderr, "[INFO] %s\n", msg.toUtf8().constData());
        break;
    }
}

void installLogHandler()
{
    qInstallMessageHandler(cliMessageHandler);
}

std::string trimRobotMessage(const std::string &msg)
{
    std::string trimmed = msg;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    return trimmed;
}
static void applyRobotMessageUnlocked(const zowi::RobotMessage &msg)
{
    if (!msg.legacy) {
        // &&-prefixed form (current firmware).
        if (msg.cmd == zowi::toChar(zowi::Command::GetName)) {
            if (msg.hasValue) {
                g_robotName = msg.value;
                g_dataReceived = true;
            }
        } else if (msg.cmd == zowi::toChar(zowi::Command::GetProgramId)) {
            if (msg.hasValue) {
                g_appId = msg.value;
                g_dataReceived = true;
            }
        } else if (msg.cmd == zowi::toChar(zowi::Command::GetBattery)) {
            if (msg.hasValue) {
                try { g_battery = std::stof(msg.value); } catch (...) {}
                g_dataReceived = true;
            }
        } else if (msg.cmd == zowi::toChar(zowi::Command::Ack)) {
            // Software ack (&&A): command received but not yet processed.
            g_ack = true;
            g_dataReceived = true;
        } else if (msg.cmd == zowi::toChar(zowi::Command::FinalAck)) {
            // Final ack (&&F): command fully processed (EEPROM write done).
            // While a movement runs the firmware also emits one &&F per
            // completed gait cycle — the move command counts on that.
            g_finalAck = true;
            ++g_finalAckCount;
            g_dataReceived = true;
        }
        return;
    }

    // Legacy line-based messages (old firmware, still parsed for compatibility).
    if (msg.cmd == zowi::toChar(zowi::Command::LegacyName)) {
        g_robotName = msg.value;
        g_dataReceived = true;
    } else if (msg.cmd == zowi::toChar(zowi::Command::LegacyProgramId)) {
        g_appId = msg.value;
        g_dataReceived = true;
    } else if (msg.cmd == zowi::toChar(zowi::Command::LegacyBattery)) {
        try { g_battery = std::stof(msg.value); } catch (...) {}
        g_dataReceived = true;
    }
}

void parseRobotMessage(const zowi::RobotMessage &msg)
{
    std::lock_guard<std::mutex> lock(g_mtx);
    applyRobotMessageUnlocked(msg);
}

void onDataReceived(const std::string &data)
{
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        if (g_uploadMode) {
            g_stkBuffer += data;
            return;
        }
    }

    std::lock_guard<std::mutex> lock(g_mtx);

    g_lastRx = std::chrono::steady_clock::now();

    if (g_debugLog) {
        std::string printable = trimRobotMessage(data);
        std::cout << "robot rx: " << printable << std::endl;
    }

    // Frame reassembly lives in the shared zowi::MessageParser; this only
    // applies the parsed messages to the global state.
    s_parser.feed(data);
    for (const auto &msg : s_parser.drain())
        applyRobotMessageUnlocked(msg);
}

} // namespace zowi_cli
