#include "zowi/robot_commands.h"
#include "zowi/protocol.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace zowi;

static int failures = 0;

static void check(const std::string &actual, const std::string &expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got '" << actual
                  << "' expected '" << expected << "'\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

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

int main()
{
    check(commandWalkForward(), "M 1 1000\r", "walkForward default");
    check(commandWalkBackward(), "M 2 1000\r", "walkBackward default");
    check(commandTurnLeft(), "M 3 1000\r", "turnLeft default");
    check(commandTurnRight(), "M 4 1000\r", "turnRight default");
    check(commandMoonwalkerLeft(), "M 6 1000 30\r", "moonwalkerLeft default");
    check(commandMoonwalkerRight(), "M 7 1000 30\r", "moonwalkerRight default");

    check(commandWalkForward(MovementSpeed::Slow), "M 1 2000\r", "walkForward slow");
    check(commandWalkForward(MovementSpeed::Fast), "M 1 700\r", "walkForward fast");
    check(commandWalkBackward(MovementSpeed::Slow), "M 2 2000\r", "walkBackward slow");
    check(commandTurnLeft(MovementSpeed::Fast), "M 3 700\r", "turnLeft fast");
    check(commandTurnRight(MovementSpeed::Medium), "M 4 1000\r", "turnRight medium");
    check(commandMoonwalkerLeft(MovementSpeed::Slow), "M 6 2000 30\r", "moonwalkerLeft slow");
    check(commandMoonwalkerRight(MovementSpeed::Fast), "M 7 700 30\r", "moonwalkerRight fast");

    check(commandStop(), "S\r", "stop");

    // Buzzer tone (T <freq> <ms>)
    check(commandTone(1000, 200), "T 1000 200\r", "tone nominal");
    check(commandTone(880, 8), "T 880 8\r", "tone bend-like");
    check(commandTone(0, 0), "T 0 0\r", "tone zeros");

    check(commandSetTrims(0, 0, 0, 0), "C 0 0 0 0\r", "trims all zero");
    check(commandSetTrims(20, 0, -8, 3), "C 20 0 -8 3\r", "trims mixed");
    check(commandSetTrims(-60, 60, -30, 30), "C -60 60 -30 30\r", "trims extremes");
    check(commandServoAt(90, 90, 90, 90), "G 90 90 90 90\r", "servo neutral");
    check(commandServoAt(150, 30, 96, 78), "G 150 30 96 78\r", "servo mixed");
    check(commandGesture(12), "H 12\r", "gesture victory");

    // Gesture enum (1-based protocol: enum 0 = protocol 1)
    check(commandGesture(GestureId::Victory), "H 12\r", "gesture victory enum");
    check(commandGesture(GestureId::Happy), "H 1\r", "gesture happy enum");
    check(commandGesture(GestureId::Fail), "H 13\r", "gesture fail enum");

    // Mouths: raw pattern
    check(commandMouth(0b00000000100001010010001100000000),
          "L 00000000100001010010001100000000\r", "mouth smile pattern");
    check(commandMouth(0), "L 00000000000000000000000000000000\r", "mouth zero pattern");
    check(commandMouth(0xFFFFFFFF), "L 11111111111111111111111111111111\r", "mouth all-on pattern");

    // Mouths: by ID
    check(commandMouthById(MouthId::Smile),
          "L 00000000100001010010001100000000\r", "mouth smile by id");
    check(commandMouthById(MouthId::Heart),
          "L 00010010101101100001010010001100\r", "mouth heart by id");
    check(commandMouthById(MouthId::Angry),
          "L 00000000011110100001100001000000\r", "mouth angry by id");
    // okMouth must be the canonical Zowi_mouths.h pattern (verified against
    // the local library, bq/zowiLibs upstream and the Bobwi fork — see
    // docs/project/ZOWILIBS.md). Locked so the "corrected" variant cannot
    // come back unnoticed.
    check(commandMouthById(MouthId::Ok),
          "L 00000001000010010100001000000000\r", "mouth ok by id (canonical)");

    // Raw binary tokens: value semantics (leading zeros optional, like the
    // firmware's strtoul(arg, nullptr, 2)); canonical 32-char form is emitted.
    std::string raw;
    checkBool(commandMouthFromBinary("00000001000010010100001000000000", raw), true, "raw 32-bit okMouth");
    check(raw, "L 00000001000010010100001000000000\r", "raw 32-bit okMouth canonical");
    checkBool(commandMouthFromBinary("101", raw), true, "raw short token");
    check(raw, "L 00000000000000000000000000000101\r", "raw 101 = value 5");
    checkBool(commandMouthFromBinary("001001001001001001001001001001", raw), true, "raw 30-digit token");
    check(raw, "L 00001001001001001001001001001001\r", "raw 30-digit zero-extended");
    checkBool(commandMouthFromBinary("1", raw), true, "raw single bit");
    check(raw, "L 00000000000000000000000000000001\r", "raw single bit canonical");
    checkBool(commandMouthFromBinary("0", raw), true, "raw zero value");
    check(raw, "L 00000000000000000000000000000000\r", "raw zero value canonical");
    checkBool(commandMouthFromBinary("", raw), false, "raw empty rejected");
    checkBool(commandMouthFromBinary("0102", raw), false, "raw non-01 rejected");
    checkBool(commandMouthFromBinary("10 1", raw), false, "raw with space rejected");
    checkBool(commandMouthFromBinary(std::string(33, '1'), raw), false, "raw 33 digits rejected");

    // Melodies: 1-based protocol (enum 0 = protocol 1)
    check(commandSing(MelodyId::Connection), "K 1\r", "sing connection");
    check(commandSing(MelodyId::Happy), "K 8\r", "sing happy");
    check(commandSing(MelodyId::ButtonPushed), "K 19\r", "sing button pushed");

    // Robot name validation (SetName argument): letters only
    checkBool(isValidRobotName("Rebecowi"), true, "name plain ascii");
    checkBool(isValidRobotName("zowi"), true, "name lowercase");
    checkBool(isValidRobotName("\xC3\x81\x72\x62\x6F\x6C"), true, "name accented (Árbol)");
    checkBool(isValidRobotName("Pe\xC3\xB1\xC3\xB3n"), true, "name ñ and ó");
    checkBool(isValidRobotName("\xC3\x87" "eca"), true, "name ç");
    checkBool(isValidRobotName(""), false, "name empty");
    checkBool(isValidRobotName("Zowi 2"), false, "name with space");
    checkBool(isValidRobotName("Zowi2"), false, "name with digit");
    checkBool(isValidRobotName("Zowi!"), false, "name with symbol");
    checkBool(isValidRobotName("Zowi\t"), false, "name with tab");
    checkBool(isValidRobotName("Zowi\xC3\x97"), false, "name with multiplication sign");
    checkBool(isValidRobotName("Zowi\xC3\xB7"), false, "name with division sign");
    checkBool(isValidRobotName("\xE6\x97\xA5"), false, "name non-latin utf8");
    checkBool(isValidRobotName("Zowi\xC3"), false, "name truncated utf8");

    if (failures == 0) {
        std::cout << "All robot_commands tests passed.\n";
        return 0;
    }
    std::cerr << failures << " robot_commands test(s) failed.\n";
    return 1;
}
