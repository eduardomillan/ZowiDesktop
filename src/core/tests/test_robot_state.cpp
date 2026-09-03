#include "zowi/robot_state.h"
#include "zowi/message_parser.h"

#include <iostream>
#include <string>

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

static void checkStr(const std::string &actual, const std::string &expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got '" << actual << "' expected '" << expected << "'\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkFloat(float actual, float expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << actual << " expected " << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

// Runs a chunk through the parser and applies every message.
static RobotState::Update feed(MessageParser &p, RobotState &s, const std::string &chunk)
{
    RobotState::Update all;
    p.feed(chunk);
    for (const auto &msg : p.drain()) {
        const auto upd = s.apply(msg);
        all.name |= upd.name;
        all.appId |= upd.appId;
        all.battery |= upd.battery;
    }
    return all;
}

int main()
{
    // ── Prefixed identity replies ──────────────────────────────────────────
    {
        MessageParser p;
        RobotState s;
        const auto upd = feed(p, s, "&&E Rebeconowi%%&&I ZOWI_BASE_v2%%&&B 85.5%%\r\n");
        checkStr(s.name, "Rebeconowi", "prefixed name");
        checkStr(s.appId, "ZOWI_BASE_v2", "prefixed app id");
        checkFloat(s.battery, 85.5f, "prefixed battery");
        checkBool(upd.name && upd.appId && upd.battery, true, "all three flagged");
    }

    // ── Present-but-empty value slot keeps the name/appId semantics ────────
    {
        MessageParser p;
        RobotState s;
        const auto upd = feed(p, s, "&&E %%");
        checkBool(upd.name, true, "empty name slot still flagged");
        checkStr(s.name, "", "empty name applied");
    }

    // ── &&E without a value slot is ignored ────────────────────────────────
    {
        MessageParser p;
        RobotState s;
        s.name = "keep";
        const auto upd = feed(p, s, "&&A%%&&E");
        checkBool(upd.name, false, "bare &&E ignored");
        checkStr(s.name, "keep", "name untouched by bare &&E");
    }

    // ── Legacy line forms ──────────────────────────────────────────────────
    {
        MessageParser p;
        RobotState s;
        const auto upd = feed(p, s, "N Rebecowi\nU App_v2\nB 90.0\n");
        checkStr(s.name, "Rebecowi", "legacy name");
        checkStr(s.appId, "App_v2", "legacy app id");
        checkFloat(s.battery, 90.0f, "legacy battery");
        checkBool(upd.name && upd.appId && upd.battery, true, "legacy all flagged");
    }

    // ── Unparseable battery: received flag, level untouched ────────────────
    {
        MessageParser p;
        RobotState s;
        s.battery = 42.0f;
        const auto upd = feed(p, s, "&&B oops%%");
        checkBool(upd.battery, true, "bad battery still flagged as received");
        checkFloat(s.battery, 42.0f, "bad battery leaves level untouched");
    }

    // ── Other commands do not touch the state ──────────────────────────────
    {
        MessageParser p;
        RobotState s;
        s.name = "Zowi";
        s.appId = "App";
        s.battery = 50.0f;
        const auto upd = feed(p, s, "&&A%%&&F%%&&D 12%%&&N 512%%");
        checkBool(upd.name || upd.appId || upd.battery, false, "acks/sensors not identity");
        checkStr(s.name, "Zowi", "name untouched");
        checkFloat(s.battery, 50.0f, "battery untouched");
    }

    // ── clear() ────────────────────────────────────────────────────────────
    {
        RobotState s;
        s.name = "x";
        s.appId = "y";
        s.battery = 10.0f;
        s.clear();
        checkStr(s.name, "", "clear name");
        checkStr(s.appId, "", "clear app id");
        checkFloat(s.battery, -1.0f, "clear battery");
    }

    // ── sendIdentityQueries sends E, I and B ───────────────────────────────
    {
        struct CaptureBackend : BluetoothApi {
            std::vector<std::string> sent;
            bool init() override { return true; }
            void startDiscovery() override {}
            void stopDiscovery() override {}
            bool connect(const std::string &) override { return true; }
            void disconnect() override {}
            bool send(const std::string &data) override { sent.push_back(data); return true; }
            bool isConnected() const override { return true; }
            std::string lastError() const override { return {}; }
            void setAutoReconnect(bool, int) override {}
            void unpair(const std::string &) override {}
        } bt;
        sendIdentityQueries(bt);
        checkStr(bt.sent.size() == 3 ? "3" : "other", "3", "three identity queries");
        checkStr(bt.sent[0], "E\r", "query name");
        checkStr(bt.sent[1], "I\r", "query app id");
        checkStr(bt.sent[2], "B\r", "query battery");
    }

    if (failures == 0) {
        std::cout << "All robot_state tests passed.\n";
        return 0;
    }
    std::cerr << failures << " robot_state test(s) failed.\n";
    return 1;
}
