#include "zowi/message_parser.h"
#include "zowi/protocol.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace zowi;

static int failures = 0;

static void checkBool(bool actual, bool expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << (actual ? "true" : "false")
                  << " expected " << (expected ? "true" : "false") << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkMsg(const RobotMessage &msg, char cmd, const std::string &value,
                     bool hasValue, bool legacy, const char *label)
{
    const bool ok = msg.cmd == cmd && msg.value == value &&
                    msg.hasValue == hasValue && msg.legacy == legacy;
    if (!ok) {
        std::cerr << "FAIL [" << label << "]: got cmd '" << msg.cmd << "' value '"
                  << msg.value << "' hasValue " << (msg.hasValue ? "true" : "false")
                  << " legacy " << (msg.legacy ? "true" : "false")
                  << " — expected cmd '" << cmd << "' value '" << value
                  << "' hasValue " << (hasValue ? "true" : "false")
                  << " legacy " << (legacy ? "true" : "false") << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

// Feeds a chunk, drains and checks that exactly `expected` messages came out.
static std::vector<RobotMessage> feedAndDrain(MessageParser &p, const std::string &chunk,
                                              size_t expected, const char *label)
{
    p.feed(chunk);
    auto msgs = p.drain();
    if (msgs.size() != expected) {
        std::cerr << "FAIL [" << label << "]: got " << msgs.size() << " message(s), expected "
                  << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << ": " << expected << " message(s)]\n";
    }
    return msgs;
}

int main()
{
    // ── parseRobotToken ────────────────────────────────────────────────────
    {
        RobotMessage msg;
        checkBool(parseRobotToken("&&E Zowi", msg), true, "token prefixed name");
        checkMsg(msg, 'E', "Zowi", true, false, "token prefixed name fields");

        checkBool(parseRobotToken("&&A", msg), true, "token bare ack");
        checkMsg(msg, 'A', "", false, false, "token bare ack fields");

        checkBool(parseRobotToken("&&I ", msg), true, "token empty value slot");
        checkMsg(msg, 'I', "", true, false, "token empty value slot fields");

        checkBool(parseRobotToken("N Rebecowi", msg), true, "token legacy name");
        checkMsg(msg, 'N', "Rebecowi", true, true, "token legacy name fields");

        checkBool(parseRobotToken("B 85.0\r", msg), true, "token legacy crlf");
        checkMsg(msg, 'B', "85.0", true, true, "token legacy crlf fields");

        checkBool(parseRobotToken("&&", msg), false, "token too short");
        checkBool(parseRobotToken("", msg), false, "token empty");
        checkBool(parseRobotToken("B", msg), false, "token legacy no space");
        checkBool(parseRobotToken("AB", msg), false, "token legacy one char value-less");
    }

    // ── Single frames ──────────────────────────────────────────────────────
    {
        MessageParser p;
        auto msgs = feedAndDrain(p, "&&E Rebecowi%%\r\n", 1, "single prefixed name");
        if (!msgs.empty()) checkMsg(msgs[0], 'E', "Rebecowi", true, false, "single prefixed name fields");

        auto acks = feedAndDrain(p, "&&A%%&&F%%", 2, "two bare acks one chunk");
        if (acks.size() == 2) {
            checkMsg(acks[0], 'A', "", false, false, "bare ack 1");
            checkMsg(acks[1], 'F', "", false, false, "bare ack 2");
        }

        auto lines = feedAndDrain(p, "N Zowi\nU ZOWI_BASE_v2\r\nB 85.0\n", 3, "legacy lines");
        if (lines.size() == 3) {
            checkMsg(lines[0], 'N', "Zowi", true, true, "legacy name fields");
            checkMsg(lines[1], 'U', "ZOWI_BASE_v2", true, true, "legacy appid fields");
            checkMsg(lines[2], 'B', "85.0", true, true, "legacy battery fields");
        }

        checkBool(p.idle(), true, "idle after clean frames");
    }

    // ── Chunked frames (split across feed() calls) ─────────────────────────
    {
        MessageParser p;
        p.feed("&&E Zo");
        checkBool(p.drain().empty(), true, "partial frame yields nothing");
        p.feed("wi%%\r");
        auto msgs = p.drain();
        checkBool(msgs.size() == 1, true, "frame completed across chunks");
        if (msgs.size() == 1) checkMsg(msgs[0], 'E', "Zowi", true, false, "chunked name fields");
        checkBool(p.idle(), true, "idle after chunked frame");
    }

    // ── Mixed prefixed + legacy, order preserved ───────────────────────────
    {
        MessageParser p;
        p.feed("  \r\n&&B 91.0%%noise\nN Other\n");
        auto msgs = p.drain();
        // "noise" is not "<cmd> <value>" → dropped; prefixed + legacy remain.
        checkBool(msgs.size() == 2, true, "noise dropped, prefixed + legacy kept");
        if (msgs.size() == 2) {
            checkMsg(msgs[0], 'B', "91.0", true, false, "prefixed battery fields");
            checkMsg(msgs[1], 'N', "Other", true, true, "legacy name after noise");
        }
    }

    // ── Unknown commands still parse (consumers decide) ────────────────────
    {
        MessageParser p;
        auto msgs = feedAndDrain(p, "&&D 12%%\n&&N 512%%\n", 2, "distance and noise");
        if (msgs.size() == 2) {
            checkMsg(msgs[0], 'D', "12", true, false, "distance fields");
            checkMsg(msgs[1], 'N', "512", true, false, "noise fields");
        }
    }

    // ── Safety valve: unterminated garbage cannot wedge the parser ─────────
    {
        MessageParser p;
        p.feed(std::string(5000, 'a')); // no newline, no %% → discarded at cap
        checkBool(p.drain().empty(), true, "garbage yields nothing");
        checkBool(p.idle(), true, "garbage buffer dropped at cap");
        p.feed("&&B 50%%\r\n"); // parser resyncs on the next real frame
        auto msgs = p.drain();
        checkBool(msgs.size() == 1, true, "resync after garbage flood");
        if (msgs.size() == 1) checkMsg(msgs[0], 'B', "50", true, false, "battery after resync");
    }

    // ── reset() ────────────────────────────────────────────────────────────
    {
        MessageParser p;
        p.feed("&&E Zowi"); // partial
        p.reset();
        checkBool(p.idle(), true, "idle after reset");
        auto msgs = feedAndDrain(p, "%%", 0, "reset dropped partial frame");
        (void)msgs;
    }

    if (failures == 0) {
        std::cout << "All message_parser tests passed.\n";
        return 0;
    }
    std::cerr << failures << " message_parser test(s) failed.\n";
    return 1;
}
