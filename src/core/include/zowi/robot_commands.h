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
// Turn left/right: firmware MoveIDs 3-4.
// Moonwalker left/right: firmware MoveIDs 6-7 (matching ZowiAppReborn's pad).
std::string commandWalkForward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandWalkBackward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnLeft(MovementSpeed speed = MovementSpeed::Medium);
std::string commandTurnRight(MovementSpeed speed = MovementSpeed::Medium);
std::string commandMoonwalkerLeft(MovementSpeed speed = MovementSpeed::Medium);
std::string commandMoonwalkerRight(MovementSpeed speed = MovementSpeed::Medium);

// Action movements (firmware MoveIDs 5, 8-20).
std::string commandUpDown(MovementSpeed speed = MovementSpeed::Medium, int size = 15);
std::string commandSwing(MovementSpeed speed = MovementSpeed::Medium, int size = 15);
std::string commandCrusaitoForward(MovementSpeed speed = MovementSpeed::Medium, int size = 30);
std::string commandCrusaitoBackward(MovementSpeed speed = MovementSpeed::Medium, int size = 30);
std::string commandJump(MovementSpeed speed = MovementSpeed::Medium);
std::string commandFlappingLeft(MovementSpeed speed = MovementSpeed::Medium, int size = 30);
std::string commandFlappingRight(MovementSpeed speed = MovementSpeed::Medium, int size = 30);
std::string commandTiptoeSwing(MovementSpeed speed = MovementSpeed::Medium, int size = 15);
std::string commandBendForward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandBendBackward(MovementSpeed speed = MovementSpeed::Medium);
std::string commandShakeLegLeft(MovementSpeed speed = MovementSpeed::Medium);
std::string commandShakeLegRight(MovementSpeed speed = MovementSpeed::Medium);
std::string commandJitter(MovementSpeed speed = MovementSpeed::Medium, int size = 15);
std::string commandAscendingTurn(MovementSpeed speed = MovementSpeed::Medium, int size = 15);

// Stop / home: moves all servos to 90 degrees and detaches them.
std::string commandStop();

// Calibration. The firmware keeps no range enforcement of its own: the only
// hard limit is the servo's physical span (0-180°), so a trim of +60 produces
// 90+60=150°. Callers are responsible for keeping values inside +/-60.
//
// commandSetTrims persists the four offsets to EEPROM (survives power cycles).
std::string commandSetTrims(int yl, int yr, int rl, int rr);

// commandServoAt moves the four servos to the given raw angles in real time
// (volatile, does not persist). For calibration use 90+trim per servo.
std::string commandServoAt(int yl, int yr, int rl, int rr);

// Gesture animation: H <id> (firmware gestos 1-13). 12 = VICTORY.
std::string commandGesture(int gestureId);

} // namespace zowi

#endif // ZOWI_ROBOT_COMMANDS_H
