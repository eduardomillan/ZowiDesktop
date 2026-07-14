#pragma once

#include <string>

namespace zowi {

// Protocol framing — every robot message / command follows:
//   &&<cmd> [<value>]%%
//   (or the legacy line-based format: <cmd> <value>\n)
constexpr char kMessagePrefix[]   = "&&";
constexpr char kMessageTerminator[] = "%%";

// Command identifiers matching the firmware's ZowiSerialCommand handlers
// (defined in zowiLibs/arduino libraries/ZowiSerialCommand/).
// See docs/project/ZOWILIBS.md for the full protocol table.
enum class Command : char {
    // ── Outgoing commands (desktop → robot) ─────────────────
    Stop       = 'S',
    LED        = 'L',
    Buzzer     = 'T',
    Move       = 'M',
    Gesture    = 'H',
    Sing       = 'K',
    Trims      = 'C',
    Servo      = 'G',
    SetName    = 'R',

    // ── Query / request (desktop → robot) ───────────────────
    // The robot replies with a &&‑prefixed message of the same char + value.
    GetName      = 'E',
    GetDistance  = 'D',
    GetNoise     = 'N',
    GetBattery   = 'B',
    GetProgramId = 'I',

    // ── Acknowledge responses (robot → desktop) ─────────────
    Ack      = 'A',  // command received
    FinalAck = 'F',  // command fully processed (after EEPROM write, etc.)

    // ── Legacy line-based protocol (old firmware) ───────────
    // These are parsed in addition to the &&‑prefixed equivalents.
    LegacyName       = 'N',
    LegacyProgramId  = 'U',
    LegacyBattery    = 'B',
};

inline char toChar(Command cmd) { return static_cast<char>(cmd); }

// Build a firmware command string: "<cmd> [args]\r".
// The firmware's ZowiSerialCommand parses up to '\r' or '\n'.
//   makeCommand(Command::Stop)                  → "S\r"
//   makeCommand(Command::Move, "1 1000")        → "M 1 1000\r"
//   makeCommand(Command::SetName, "TestZowi")   → "R TestZowi\r"
//   makeCommand(Command::GetProgramId)           → "I\r"
inline std::string makeCommand(Command cmd, const std::string &args = {})
{
    std::string out(1, toChar(cmd));
    if (!args.empty()) {
        out += ' ';
        out += args;
    }
    out += '\r';
    return out;
}

// Returns the command character from a raw &&-prefixed message token,
// e.g. prefixedCommandChar("&&I ZOWI_Alarm_v2") → 'I'.
// Returns '\0' if the token does not start with the protocol prefix.
inline char prefixedCommandChar(const std::string &token)
{
    if (token.size() >= 2 && token[0] == kMessagePrefix[0] && token[1] == kMessagePrefix[1])
        return token.size() > 2 ? token[2] : '\0';
    return '\0';
}

// Returns the value portion of a &&-prefixed message, i.e. everything after
// "&&X " (where X = command char). Returns empty string on mismatch.
//   prefixedCommandValue("&&E Zowi")  → "Zowi"
//   prefixedCommandValue("&&I App_v2") → "App_v2"
inline std::string prefixedCommandValue(const std::string &token)
{
    // Format: &&<cmd> <value>
    if (token.size() < 4) return {};
    if (token[0] != kMessagePrefix[0] || token[1] != kMessagePrefix[1]) return {};
    if (token[3] != ' ') return {};
    return token.substr(4);
}

} // namespace zowi
