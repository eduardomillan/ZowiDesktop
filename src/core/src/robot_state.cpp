#include "zowi/robot_state.h"

#include <zowi/protocol.h>

namespace zowi {

namespace {

void storeBattery(float &dst, const std::string &value)
{
    try {
        dst = std::stof(value);
    } catch (...) {
        // Unparseable value: reported as received, level left untouched.
    }
}

} // namespace

RobotState::Update RobotState::apply(const RobotMessage &msg)
{
    Update upd;

    if (msg.legacy) {
        // Legacy line forms of the old firmware: N name / U app id / B battery.
        switch (msg.cmd) {
        case toChar(Command::LegacyName):
            name = msg.value;
            upd.name = true;
            break;
        case toChar(Command::LegacyProgramId):
            appId = msg.value;
            upd.appId = true;
            break;
        case toChar(Command::LegacyBattery):
            upd.battery = true;
            storeBattery(battery, msg.value);
            break;
        default:
            break;
        }
        return upd;
    }

    switch (msg.cmd) {
    case toChar(Command::GetName):
        if (msg.hasValue) {
            name = msg.value;
            upd.name = true;
        }
        break;
    case toChar(Command::GetProgramId):
        if (msg.hasValue) {
            appId = msg.value;
            upd.appId = true;
        }
        break;
    case toChar(Command::GetBattery):
        if (msg.hasValue) {
            upd.battery = true;
            storeBattery(battery, msg.value);
        }
        break;
    default:
        break;
    }
    return upd;
}

void RobotState::clear()
{
    name.clear();
    appId.clear();
    battery = -1.0f;
}

void sendIdentityQueries(BluetoothApi &bt)
{
    bt.send(makeCommand(Command::GetName));
    bt.send(makeCommand(Command::GetProgramId));
    bt.send(makeCommand(Command::GetBattery));
}

} // namespace zowi
