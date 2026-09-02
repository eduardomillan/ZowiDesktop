#include "zowi/calibration_session.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace zowi;

static int failures = 0;

static void check(bool cond, const char *label)
{
    if (!cond) {
        std::cerr << "FAIL [" << label << "]\n";
        ++failures;
    }
}

static void checkStr(const std::string &actual, const std::string &expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got '" << actual
                  << "' expected '" << expected << "'\n";
        ++failures;
    }
}

void test_clamp()
{
    std::cout << "test_clamp: " << std::flush;
    CalibrationSession s;
    check(s.clamp(0) == 0, "clamp 0");
    check(s.clamp(-60) == -60, "clamp min bound");
    check(s.clamp(60) == 60, "clamp max bound");
    check(s.clamp(61) == 60, "clamp above max");
    check(s.clamp(-61) == -60, "clamp below min");
    std::cout << "OK\n";
}

void test_adjust_and_trim()
{
    std::cout << "test_adjust_and_trim: " << std::flush;
    CalibrationSession s;
    check(s.adjust(CalibrationSession::LeftLeg, 5), "adjust +5 changed");
    check(s.trim(CalibrationSession::LeftLeg) == 5, "trim readback");
    check(s.adjust(CalibrationSession::RightLeg, 70), "adjust +70 clamps to 60");
    check(s.trim(CalibrationSession::RightLeg) == 60, "trim clamped");
    check(!s.adjust(CalibrationSession::RightLeg, 10), "adjust at max returns false");
    check(s.adjust(CalibrationSession::RightLeg, -200) == true, "adjust big negative clamps");
    check(s.trim(CalibrationSession::RightLeg) == -60, "trim clamped negative");
    // LeftLeg untouched by other-column adjusts.
    check(s.trim(CalibrationSession::RightFoot) == 0, "other trim untouched");
    std::cout << "OK\n";
}

void test_commands()
{
    std::cout << "test_commands: " << std::flush;
    CalibrationSession s;
    checkStr(s.neutralCommand(), "G 90 90 90 90\r", "neutral");
    s.adjust(CalibrationSession::LeftLeg, 5);
    s.adjust(CalibrationSession::RightFoot, -3);
    checkStr(s.servosCommand(), "G 95 90 90 87\r", "servos 90+trim");
    checkStr(s.trimsCommand(), "C 5 0 0 -3\r", "trims");
    s.reset();
    checkStr(s.servosCommand(), "G 90 90 90 90\r", "servos after reset");
    std::cout << "OK\n";
}

void test_steps()
{
    std::cout << "test_steps: " << std::flush;
    CalibrationSession s;
    check(s.stepIndex() == 0, "starts at Warning");
    s.nextStep();
    check(s.stepIndex() == 1, "Legs");
    s.nextStep();
    check(s.stepIndex() == 2, "Feet");
    s.nextStep();
    check(s.stepIndex() == 3, "Check");
    s.nextStep();  // stays on Check
    check(s.stepIndex() == 3, "stays on Check");
    s.setStepIndex(1);
    check(s.stepIndex() == 1, "setStepIndex jumps to Legs");
    s.setStepIndex(99);
    check(s.stepIndex() == 3, "setStepIndex clamps to Check");
    s.reset();
    check(s.stepIndex() == 0, "reset returns to Warning");
    std::cout << "OK\n";
}

void test_should_send_debounce()
{
    std::cout << "test_should_send_debounce: " << std::flush;
    CalibrationSession s;
    long long last = 0;
    check(s.shouldSend(1000, last), "first send passes");
    check(last == 1000, "lastSendMs updated");
    // Inside the debounce window: dropped.
    check(!s.shouldSend(1099, last), "coalesced (too soon)");
    check(!s.shouldSend(1199, last), "coalesced (still too soon)");
    // At the boundary (200ms later): allowed.
    check(s.shouldSend(1200, last), "passes after debounce");
    check(last == 1200, "lastSendMs advanced");
    std::cout << "OK\n";
}

int main()
{
    test_clamp();
    test_adjust_and_trim();
    test_commands();
    test_steps();
    test_should_send_debounce();

    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "All tests passed.\n";
    return 0;
}