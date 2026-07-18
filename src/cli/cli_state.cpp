#include "cli_state.h"

#include <algorithm>
#include <zowi/protocol.h>

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
std::string g_dataBuffer;
bool g_uploadMode = false;
std::string g_stkBuffer;
std::atomic<bool> g_quit{false};
int g_stdinFd = STDIN_FILENO;

const float kLowBatteryThreshold = 50.0f;
const char *const kFactoryFirmwarePath = "src/firmware/ZOWI_BASE_v2.hex";
const char *const kAlarmFirmwarePath = "src/firmware/ZOWI_Alarm_v2.hex";
const char *const kAdivinawiFirmwarePath = "src/firmware/ZOWI_Adivinawi_v2.hex";
const int kDiscoveryTimeoutMs = 6000;

void resetRobotState()
{
    std::lock_guard<std::mutex> lock(g_mtx);
    g_robotName.clear();
    g_appId.clear();
    g_battery = -1.0f;
    g_connected = false;
    g_connectedOnce = false;
    g_dataReceived = false;
    g_ack = false;
    g_finalAck = false;
    g_dataBuffer.clear();
}

std::string trimRobotMessage(const std::string &msg)
{
    std::string trimmed = msg;
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\r'), trimmed.end());
    trimmed.erase(std::remove(trimmed.begin(), trimmed.end(), '\n'), trimmed.end());
    return trimmed;
}

static void parseRobotMessageUnlocked(const std::string &msg)
{
    std::string trimmed = trimRobotMessage(msg);
    if (trimmed.empty()) return;

    // &&‑prefixed message: &&<cmd>[ <value>]
    if (trimmed.size() >= 2 && trimmed[0] == zowi::kMessagePrefix[0]
                             && trimmed[1] == zowi::kMessagePrefix[1]) {
        char prefix = trimmed[2];

        if (trimmed.size() >= 4 && trimmed[3] == ' ') {
            std::string value = trimmed.substr(4);
            if (prefix == zowi::toChar(zowi::Command::GetName)) {
                g_robotName = value;
                g_dataReceived = true;
            } else if (prefix == zowi::toChar(zowi::Command::GetProgramId)) {
                g_appId = value;
                g_dataReceived = true;
            } else if (prefix == zowi::toChar(zowi::Command::GetBattery)) {
                try { g_battery = std::stof(value); } catch (...) {}
                g_dataReceived = true;
            } else if (prefix == zowi::toChar(zowi::Command::Ack)) {
                // Software ack (&&A): command received but not yet processed.
                g_ack = true;
                g_dataReceived = true;
            } else if (prefix == zowi::toChar(zowi::Command::FinalAck)) {
                // Final ack (&&F): command fully processed (EEPROM write done).
                g_finalAck = true;
                g_dataReceived = true;
            }
        } else if (prefix == zowi::toChar(zowi::Command::Ack)) {
            // Bare &&A / &&F (no value, no space) — the firmware sends this
            // when the command (e.g. rename) has been fully processed.
            g_ack = true;
            g_dataReceived = true;
        } else if (prefix == zowi::toChar(zowi::Command::FinalAck)) {
            g_finalAck = true;
            g_dataReceived = true;
        }
        return;
    }

    // Legacy line-based messages (old firmware, still parsed for compatibility).
    if (trimmed[0] == zowi::toChar(zowi::Command::LegacyName) && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_robotName = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == zowi::toChar(zowi::Command::LegacyProgramId) && trimmed.size() > 2 && trimmed[1] == ' ') {
        g_appId = trimmed.substr(2);
        g_dataReceived = true;
    } else if (trimmed[0] == zowi::toChar(zowi::Command::LegacyBattery) && trimmed.size() > 2 && trimmed[1] == ' ') {
        try { g_battery = std::stof(trimmed.substr(2)); } catch (...) {}
        g_dataReceived = true;
    }
}

void parseRobotMessage(const std::string &msg)
{
    parseRobotMessageUnlocked(msg);
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
    g_dataBuffer += data;

    // Robot protocol can arrive either as &&E <name>%% / &&I <appId>%% / &&B <battery>%%
    // or as line-based N / U / B messages.
    while (true) {
        auto start = g_dataBuffer.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            g_dataBuffer.clear();
            break;
        }
        if (start > 0) {
            g_dataBuffer.erase(0, start);
        }

        if (g_dataBuffer.rfind(zowi::kMessagePrefix, 0) == 0) {
            auto tokenEnd = g_dataBuffer.find(zowi::kMessageTerminator);
            if (tokenEnd == std::string::npos) break;

            std::string token = g_dataBuffer.substr(0, tokenEnd);
            g_dataBuffer.erase(0, tokenEnd + (sizeof(zowi::kMessageTerminator) - 1));
            parseRobotMessageUnlocked(token);
            continue;
        }

        auto lineEnd = g_dataBuffer.find('\n');
        if (lineEnd == std::string::npos) break;

        std::string line = g_dataBuffer.substr(0, lineEnd);
        g_dataBuffer.erase(0, lineEnd + 1);
        parseRobotMessageUnlocked(line);
    }
}

} // namespace zowi_cli
