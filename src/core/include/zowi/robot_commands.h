#ifndef ZOWI_ROBOT_COMMANDS_H
#define ZOWI_ROBOT_COMMANDS_H

#include <string>

namespace zowi {

// Builds the serial command strings understood by the Zowi firmware
// (see docs/firmware/PROTOCOL.md). Every command is space-delimited and
// terminated by a carriage return ('\r'), matching the firmware's
// ZowiSerialCommand parser.
//
// This helper is intentionally Qt-free so it can be shared by both the CLI
// and the GUI control pad (M3) while staying testable in src/core/tests.

// Speed presets map to the firmware's period parameter T (ms). Larger = slower.
// Values match ZowiAppReborn's speed options.
enum class MovementSpeed {
    Slow = 2000,
    Medium = 1000,
    Fast = 700
};

// Directional movement. Each command runs one gait cycle; re-send to keep moving.
// Walk forward/backward: firmware MoveIDs 1-2.
// Left/right default to moonwalker (MoveIDs 6-7) matching ZowiAppReborn's pad.
std::string commandWalkForward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandWalkBackward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnLeft(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnRight(MovementSpeed speed = MovementSpeed::Medium);

// Stop / home: moves all servos to 90 degrees and detaches them.
std::string commandStop();

} // namespace zowi

#endif // ZOWI_ROBOT_COMMANDS_H
