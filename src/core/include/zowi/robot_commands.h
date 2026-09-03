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

// Buzzer tone: T <freqHz> <durationMs>\r. The firmware's recieveBuzzer()
// atoi()s both arguments and calls zowi._tone(freq, duration, 1).
std::string commandTone(int frequencyHz, int durationMs);

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

// ── Mouths / LED matrix ─────────────────────────────────────────────────────
// The firmware's L command takes a 32-bit binary pattern (e.g. "L 00000000100001010010001100000000\r"
// for a smile). The patterns are defined in zowiLibs/arduinolibs/Zowi/Zowi_mouths.h.
// We expose both the raw pattern command and named mouth IDs.

// Mouth IDs (0-30). The protocol is 0-based for mouths (unlike gestures/melodies).
enum class MouthId : int {
    Zero = 0, One, Two, Three, Four, Five, Six, Seven, Eight, Nine,
    Smile = 10, HappyOpen, HappyClosed, Heart,
    BigSurprise, SmallSurprise, TongueOut,
    Vamp1, Vamp2, LineMouth, Confused, Diagonal,
    Sad, SadOpen, SadClosed, Ok, X, Interrogation,
    Thunder, Culito, Angry
};

// Build "L <binary>\r" — the mouth command takes the raw 32-bit pattern.
std::string commandMouth(unsigned long matrix);

// Build "L <binary>\r" for a named mouth ID (looks up the pattern).
std::string commandMouthById(MouthId id);

// ── Gestures (enum overload) ────────────────────────────────────────────────
// Gesture IDs (protocol is 1-based: 1=Happy..13=Fail).
// The enum values are 0-based; the command adds 1 for the protocol.
enum class GestureId : int {
    Happy = 0, SuperHappy, Sad, Sleeping, Fart, Confused,
    Love, Angry, Fretful, Magic, Wave, Victory, Fail
};

// Overload: commandGesture(GestureId) → H <id+1>\r
std::string commandGesture(GestureId id);

// ── Melodies / Sing ─────────────────────────────────────────────────────────
// Melody IDs (protocol is 1-based: 1=Connection..19=ButtonPushed).
//
// NOTE: the enum follows the order of the firmware's receiveSing() switch in
// ZOWI_BASE_v2.ino (K 1 → S_connection ... K 19 → S_buttonPushed), which is
// NOT the raw order of the S_* defines in zowiLibs/arduinolibs/Zowi/
// Zowi_sounds.h (connection, disconnection, buttonPushed, mode1-3, surprise,
// ...). The wire order is what matters; scripts/verify_arduino_mirrors.sh
// checks this mapping stays in sync.
//
// The enum values are 0-based; the command adds 1 for the protocol.
enum class MelodyId : int {
    Connection = 0, Disconnection, Surprise, OhOoh, OhOoh2,
    Cuddly, Sleeping, Happy, SuperHappy, HappyShort,
    Sad, Confused, Fart1, Fart2, Fart3,
    Mode1, Mode2, Mode3, ButtonPushed
};

// Build "K <id+1>\r" — the sing command (1-based protocol).
std::string commandSing(MelodyId id);

} // namespace zowi

#endif // ZOWI_ROBOT_COMMANDS_H
