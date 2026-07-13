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
enum class MovementSpeed {
    Slow = 1600,
    Medium = 1000,
    Fast = 600
};

// Directional movement (firmware MoveIDs 1-4). Each command runs one gait
// cycle; re-send to keep moving.
std::string commandWalkForward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandWalkBackward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnLeft(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnRight(MovementSpeed speed = MovementSpeed::Medium);

// Stop / home: moves all servos to 90 degrees and detaches them.
std::string commandStop();

} // namespace zowi

#endif // ZOWI_ROBOT_COMMANDS_H
