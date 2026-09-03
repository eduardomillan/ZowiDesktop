#pragma once

#include <string>
#include <vector>

namespace zowi {

// One parsed robot → host message.
//
// Prefixed form (current firmware), legacy == false:
//   &&A%%        → cmd 'A', value "", hasValue false   (software ack)
//   &&F%%        → cmd 'F', value "", hasValue false   (final ack)
//   &&E Zowi%%   → cmd 'E', value "Zowi", hasValue true (robot name)
//   &&I %%       → cmd 'I', value "", hasValue true    (present-but-empty value)
// Legacy line form (old firmware), legacy == true:
//   N Rebecowi   → cmd 'N', value "Rebecowi", hasValue true (name)
//   U App_v2     → cmd 'U', value "App_v2",   hasValue true (program id)
//   B 85.0       → cmd 'B', value "85.0",     hasValue true (battery)
//
// The cmd letters overlap between forms with different meanings: a legacy 'N'
// is the robot name while a prefixed 'N' is the noise level, mirroring the
// firmware (ZOWI_BASE_v2.ino legacy senders vs &&-framed requestNoise).
struct RobotMessage {
    char cmd = '\0';
    std::string value;
    bool hasValue = false;  // "<cmd> <value>" shape (the space slot was present)
    bool legacy = false;
};

// Host-side inverse of the firmware's ZowiSerialCommand parser (see
// docs/project/ZOWILIBS.md, "Mirroring the Arduino libraries"). Feed raw
// chunks as they arrive from the transport; complete frames become
// RobotMessages and incomplete frames are kept until their terminator
// arrives, so chunk boundaries never split a message.
//
//   - Prefixed frames: "&&...%%" (kMessagePrefix / kMessageTerminator).
//   - Legacy lines: everything up to '\n' (a '\r' before it is tolerated).
//   - Leading whitespace / line-ending noise between frames is skipped; a
//     line that does not look like "<cmd> <value>" is dropped.
//   - Safety valve: if more than kMaxPendingBytes pile up without a
//     terminator (broken stream / pure garbage), the pending buffer is
//     discarded instead of growing forever.
//
// Qt-free and single-threaded: call feed()/drain() from one context (the CLI
// under its state mutex, the GUI inside its Qt-queued data callback).
class MessageParser {
public:
    // Appends a raw transport chunk and extracts any complete frames.
    void feed(const std::string &chunk);

    // Returns the messages parsed so far (in arrival order) and clears them.
    std::vector<RobotMessage> drain();

    // True when no parsed messages are waiting and no partial frame is
    // buffered.
    bool idle() const;

    // Drops any buffered partial frame and pending messages (used on
    // (re)connect, mirroring the CLI's resetRobotState()).
    void reset();

private:
    void extract();

    std::string m_buffer;
    std::vector<RobotMessage> m_pending;
};

// Parses a single already-delimited token — a "&&<cmd>[ <value>]" frame body
// (terminator removed) or a legacy "<cmd> <value>" line — into `out`.
// Returns false for tokens that are not well-formed messages. Used internally
// by MessageParser and handy for tests.
bool parseRobotToken(const std::string &token, RobotMessage &out);

} // namespace zowi
