#include "zowi/message_parser.h"

#include <zowi/protocol.h>

namespace zowi {

namespace {

// Safety valve: a real frame is at most ~35 bytes (the firmware's
// SERIALCOMMANDBUFFER); anything pending well past this is a broken stream.
constexpr size_t kMaxPendingBytes = 4096;

std::string stripLineEndings(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    for (const char c : s) {
        if (c != '\r' && c != '\n') out += c;
    }
    return out;
}

} // namespace

bool parseRobotToken(const std::string &token, RobotMessage &out)
{
    const std::string t = stripLineEndings(token);

    // Prefixed frame body: "&&<cmd>[ <value>]" (&&A%% → body "&&A").
    if (t.size() >= 3 && t[0] == kMessagePrefix[0] && t[1] == kMessagePrefix[1]) {
        out.cmd = t[2];
        out.legacy = false;
        if (t.size() > 3 && t[3] == ' ') {
            out.hasValue = true;
            out.value = t.substr(4);
        } else {
            out.hasValue = false;
            out.value.clear();
        }
        return true;
    }

    // Legacy line: "<cmd> <value>".
    if (t.size() > 2 && t[1] == ' ') {
        out.cmd = t[0];
        out.legacy = true;
        out.hasValue = true;
        out.value = t.substr(2);
        return true;
    }

    return false;
}

void MessageParser::feed(const std::string &chunk)
{
    m_buffer += chunk;
    extract();
}

std::vector<RobotMessage> MessageParser::drain()
{
    std::vector<RobotMessage> out;
    out.swap(m_pending);
    return out;
}

bool MessageParser::idle() const
{
    return m_buffer.empty() && m_pending.empty();
}

void MessageParser::reset()
{
    m_buffer.clear();
    m_pending.clear();
}

void MessageParser::extract()
{
    while (true) {
        const auto start = m_buffer.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            m_buffer.clear();
            break;
        }
        if (start > 0) m_buffer.erase(0, start);

        if (m_buffer.rfind(kMessagePrefix, 0) == 0) {
            const auto end = m_buffer.find(kMessageTerminator);
            if (end == std::string::npos) break; // incomplete frame, wait

            RobotMessage msg;
            if (parseRobotToken(m_buffer.substr(0, end), msg))
                m_pending.push_back(std::move(msg));
            m_buffer.erase(0, end + (sizeof(kMessageTerminator) - 1));
            continue;
        }

        const auto nl = m_buffer.find('\n');
        if (nl == std::string::npos) break; // incomplete line, wait

        RobotMessage msg;
        if (parseRobotToken(m_buffer.substr(0, nl), msg))
            m_pending.push_back(std::move(msg));
        m_buffer.erase(0, nl + 1);
    }

    // Broken stream / garbage that never terminates: drop it so the parser
    // can resynchronise on the next real frame.
    if (m_buffer.size() > kMaxPendingBytes)
        m_buffer.clear();
}

} // namespace zowi
