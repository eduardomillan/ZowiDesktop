#include "zowi/robot_commands.h"
#include "zowi/protocol.h"

#include <sstream>

namespace zowi {

namespace {
// Movement command: M <MoveID> <T> [<MoveSize>]\r
// moveSize is only used by certain MoveIDs (6-14, 19-20); pass 0 to omit.
std::string buildMovement(int moveId, int periodMs, int moveSize = 0)
{
    std::string args = std::to_string(moveId) + ' ' + std::to_string(periodMs);
    if (moveSize > 0) {
        args += ' ' + std::to_string(moveSize);
    }
    return makeCommand(Command::Move, args);
}
} // namespace

std::string commandWalkForward(MovementSpeed speed)
{
    return buildMovement(1, static_cast<int>(speed));
}

std::string commandWalkBackward(MovementSpeed speed)
{
    return buildMovement(2, static_cast<int>(speed));
}

std::string commandTurnLeft(MovementSpeed speed)
{
    return buildMovement(3, static_cast<int>(speed));
}

std::string commandTurnRight(MovementSpeed speed)
{
    return buildMovement(4, static_cast<int>(speed));
}

std::string commandMoonwalkerLeft(MovementSpeed speed)
{
    return buildMovement(6, static_cast<int>(speed), 30);
}

std::string commandMoonwalkerRight(MovementSpeed speed)
{
    return buildMovement(7, static_cast<int>(speed), 30);
}

std::string commandUpDown(MovementSpeed speed, int size)
{
    return buildMovement(5, static_cast<int>(speed), size);
}

std::string commandSwing(MovementSpeed speed, int size)
{
    return buildMovement(8, static_cast<int>(speed), size);
}

std::string commandCrusaitoForward(MovementSpeed speed, int size)
{
    return buildMovement(9, static_cast<int>(speed), size);
}

std::string commandCrusaitoBackward(MovementSpeed speed, int size)
{
    return buildMovement(10, static_cast<int>(speed), size);
}

std::string commandJump(MovementSpeed speed)
{
    return buildMovement(11, static_cast<int>(speed));
}

std::string commandFlappingLeft(MovementSpeed speed, int size)
{
    return buildMovement(12, static_cast<int>(speed), size);
}

std::string commandFlappingRight(MovementSpeed speed, int size)
{
    return buildMovement(13, static_cast<int>(speed), size);
}

std::string commandTiptoeSwing(MovementSpeed speed, int size)
{
    return buildMovement(14, static_cast<int>(speed), size);
}

std::string commandBendForward(MovementSpeed speed)
{
    return buildMovement(15, static_cast<int>(speed));
}

std::string commandBendBackward(MovementSpeed speed)
{
    return buildMovement(16, static_cast<int>(speed));
}

std::string commandShakeLegLeft(MovementSpeed speed)
{
    return buildMovement(17, static_cast<int>(speed));
}

std::string commandShakeLegRight(MovementSpeed speed)
{
    return buildMovement(18, static_cast<int>(speed));
}

std::string commandJitter(MovementSpeed speed, int size)
{
    return buildMovement(19, static_cast<int>(speed), size);
}

std::string commandAscendingTurn(MovementSpeed speed, int size)
{
    return buildMovement(20, static_cast<int>(speed), size);
}

std::string commandStop()
{
    return makeCommand(Command::Stop);
}

// Buzzer: T <freq> <ms>\r (firmware recieveBuzzer → zowi._tone(freq, ms, 1)).
std::string commandTone(int frequencyHz, int durationMs)
{
    return makeCommand(Command::Buzzer,
                       std::to_string(frequencyHz) + ' ' + std::to_string(durationMs));
}

// Calibration: C <YL> <YR> <RL> <RR>\r — persists the trims to EEPROM.
std::string commandSetTrims(int yl, int yr, int rl, int rr)
{
    std::string args = std::to_string(yl) + ' ' + std::to_string(yr) + ' '
                     + std::to_string(rl) + ' ' + std::to_string(rr);
    return makeCommand(Command::Trims, args);
}

// Calibration: G <YL> <YR> <RL> <RR>\r — positions the servos in real time.
std::string commandServoAt(int yl, int yr, int rl, int rr)
{
    std::string args = std::to_string(yl) + ' ' + std::to_string(yr) + ' '
                     + std::to_string(rl) + ' ' + std::to_string(rr);
    return makeCommand(Command::Servo, args);
}

// Gesture: H <id>\r (firmware gestos 1-13; 12 = VICTORY).
std::string commandGesture(int gestureId)
{
    return makeCommand(Command::Gesture, std::to_string(gestureId));
}

// Gesture enum overload: H <id+1>\r (protocol is 1-based, enum is 0-based).
std::string commandGesture(GestureId id)
{
    return makeCommand(Command::Gesture, std::to_string(static_cast<int>(id) + 1));
}

// ── Mouths / LED matrix ─────────────────────────────────────────────────────
// Build "L <binary>\r" — converts the 32-bit pattern to a 32-char binary string.
std::string commandMouth(unsigned long matrix)
{
    std::string binary(32, '0');
    for (int i = 31; i >= 0; --i) {
        if (matrix & (1UL << i)) {
            binary[31 - i] = '1';
        }
    }
    return makeCommand(Command::LED, binary);
}

// Mouth patterns from zowiLibs/arduinolibs/Zowi/Zowi_mouths.h
namespace {
constexpr unsigned long kMouthPatterns[] = {
    0b00001100010010010010010010001100, // 0: zero
    0b00000100001100000100000100001110, // 1: one
    0b00001100010010000100001000011110, // 2: two
    0b00001100010010000100010010001100, // 3: three
    0b00010010010010011110000010000010, // 4: four
    0b00011110010000011100000010011100, // 5: five
    0b00000100001000011100010010001100, // 6: six
    0b00011110000010000100001000010000, // 7: seven
    0b00001100010010001100010010001100, // 8: eight
    0b00001100010010001110000010001110, // 9: nine
    0b00000000100001010010001100000000, // 10: smile
    0b00000000111111010010001100000000, // 11: happyOpen
    0b00000000111111011110000000000000, // 12: happyClosed
    0b00010010101101100001010010001100, // 13: heart
    0b00001100010010100001010010001100, // 14: bigSurprise
    0b00000000000000001100001100000000, // 15: smallSurprise
    0b00111111001001001001000110000000, // 16: tongueOut
    0b00111111101101101101010010000000, // 17: vamp1
    0b00111111101101010010000000000000, // 18: vamp2
    0b00000000000000111111000000000000, // 19: lineMouth
    0b00000000001000010101100010000000, // 20: confused
    0b00100000010000001000000100000010, // 21: diagonal
    0b00000000001100010010100001000000, // 22: sad
    0b00000000001100010010111111000000, // 23: sadOpen
    0b00000000001100011110110011000000, // 24: sadClosed
    0b00000001000010010100001000000000, // 25: okMouth (canonical Zowi_mouths.h
                                        //     pattern — see docs/project/ZOWILIBS.md)
    0b00100001010010001100010010100001, // 26: xMouth
    0b00001100010010000100000100000100, // 27: interrogation
    0b00000100001000011100001000010000, // 28: thunder
    0b00000000100001101101010010000000, // 29: culito
    0b00000000011110100001100001000000  // 30: angry
};
} // namespace

std::string commandMouthById(MouthId id)
{
    const int idx = static_cast<int>(id);
    if (idx < 0 || idx > 30) {
        return commandMouth(0); // fallback: empty/zero pattern
    }
    return commandMouth(kMouthPatterns[idx]);
}

// Raw binary token → canonical "L <32 bits>\r". Value semantics: missing
// digits are leading zeros, matching the firmware's strtoul(..., 2) parse.
bool commandMouthFromBinary(const std::string &bits, std::string &cmd)
{
    if (bits.empty() || bits.size() > 32) return false;
    if (bits.find_first_not_of("01") != std::string::npos) return false;
    cmd = commandMouth(std::stoul(bits, nullptr, 2));
    return true;
}

// ── Melodies / Sing ─────────────────────────────────────────────────────────
// Build "K <id+1>\r" — the sing command (1-based protocol, 0-based enum).
std::string commandSing(MelodyId id)
{
    return makeCommand(Command::Sing, std::to_string(static_cast<int>(id) + 1));
}

} // namespace zowi
