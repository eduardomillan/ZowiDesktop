#pragma once

#include <string>

#include <zowi/bluetooth_api.h>
#include <zowi/message_parser.h>

namespace zowi {

// Canonical robot identity/battery state, updated from parsed messages.
//
// This is the single copy of the value-application rules (which command
// maps to which field, hasValue semantics, legacy line forms, battery
// parsing) that used to be duplicated between the CLI's
// applyRobotMessageUnlocked() and the GUI's parseIncoming(). Both apply()
// everything the MessageParser yields and react to the returned flags:
// the CLI mirrors the fields into its globals, the GUI into Qt properties.
struct RobotState {
    std::string name;
    std::string appId;
    float battery = -1.0f;

    struct Update {
        bool name = false;
        bool appId = false;
        bool battery = false;  // a battery value was received (see below)
    };

    // Applies one parsed message and reports which fields received new
    // values. Prefixed &&E/&&I/&&B apply when the value slot is present
    // (even empty for name/appId, mirroring the firmware's replies); legacy
    // N/U/B lines apply their value. An unparseable battery value is
    // reported as received but leaves the stored level untouched.
    Update apply(const RobotMessage &msg);

    void clear();
};

// Sends the identity request burst (name, program id, battery) — the same
// three queries the CLI's ready-wait and the GUI's data poll have always
// sent, now from one place.
void sendIdentityQueries(BluetoothApi &bt);

} // namespace zowi
