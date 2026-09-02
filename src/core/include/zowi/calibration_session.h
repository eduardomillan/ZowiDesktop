#ifndef ZOWI_CALIBRATION_SESSION_H
#define ZOWI_CALIBRATION_SESSION_H

#include <string>

namespace zowi {

// Shared servo-calibration domain model used by both the CLI calibrate wizard
// and the GUI CalibrationScreen. Centralises the constants, trim state, clamp
// rules, command rendering and the "one G in flight" debounce policy so the two
// front-ends cannot drift apart on range/order (see docs/firmware/PROTOCOL.md).
//
// This class is intentionally Qt-free (no QObject) so it stays in src/core and
// remains testable in src/core/tests. Front-ends own only their presentation
// (terminal vs QML) and transport I/O.
class CalibrationSession {
public:
    // Servo model: rest position is 90; trims are clamped to ±60 (the servos' 0-180
    // physical span is the only hard limit, see robot_commands.h).
    static constexpr int kBaseGrade = 90;
    static constexpr int kMinTrim = -60;
    static constexpr int kMaxTrim = 60;
    // The firmware moves each servo for ~200 ms (receiveServo/_moveServos); keep
    // at most one live G in flight and let that much elapse between sends.
    static constexpr int kDebounceMs = 200;

    // Servo order (matches the C/G protocol): YL, YR, RL, RR.
    enum TrimIndex { LeftLeg = 0, RightLeg = 1, LeftFoot = 2, RightFoot = 3 };

    enum class Step { Warning, Legs, Feet, Check };

    CalibrationSession() = default;

    void reset();                                  // trims to 0, step to Warning
    int clamp(int value) const;
    int trim(int index) const;
    bool adjust(int index, int delta);             // clamp + apply; false if clamped no-op
    void nextStep();                               // Warning → Legs → Feet → Check
    int stepIndex() const;                         // 0..3 for a StackLayout / switch
    void setStepIndex(int index);                  // clamped to 0..3 (preview/jump)

    std::string servosCommand() const;             // G 90+trim x4\r (live, volatile)
    std::string trimsCommand() const;              // C trim x4\r (persist to EEPROM)
    std::string neutralCommand() const;            // G 90 90 90 90\r

    // One-G-in-flight policy. Returns true when at least kDebounceMs has elapsed
    // since *lastSendMs and updates it (callers then send servosCommand());
    // returns false to coalesce (callers drop the intermediate value — the most
    // recent live value is already held in the session trims, so a later
    // shouldSend that passes emits it).
    bool shouldSend(long long nowMs, long long &lastSendMs) const;

private:
    int m_trims[4]{0, 0, 0, 0};
    Step m_step = Step::Warning;
};

} // namespace zowi

#endif // ZOWI_CALIBRATION_SESSION_H