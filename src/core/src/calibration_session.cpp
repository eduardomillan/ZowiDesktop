#include "zowi/calibration_session.h"
#include "zowi/robot_commands.h"

namespace zowi {

void CalibrationSession::reset()
{
    m_trims[0] = m_trims[1] = m_trims[2] = m_trims[3] = 0;
    m_step = Step::Warning;
}

int CalibrationSession::clamp(int value) const
{
    if (value < kMinTrim) return kMinTrim;
    if (value > kMaxTrim) return kMaxTrim;
    return value;
}

int CalibrationSession::trim(int index) const
{
    return m_trims[index];
}

bool CalibrationSession::adjust(int index, int delta)
{
    const int clamped = clamp(m_trims[index] + delta);
    if (clamped == m_trims[index]) return false;
    m_trims[index] = clamped;
    return true;
}

void CalibrationSession::nextStep()
{
    if (m_step == Step::Warning) m_step = Step::Legs;
    else if (m_step == Step::Legs) m_step = Step::Feet;
    else if (m_step == Step::Feet) m_step = Step::Check;
}

int CalibrationSession::stepIndex() const
{
    switch (m_step) {
        case Step::Warning: return 0;
        case Step::Legs: return 1;
        case Step::Feet: return 2;
        case Step::Check: return 3;
    }
    return 0;
}

void CalibrationSession::setStepIndex(int index)
{
    switch (index) {
        case 0: m_step = Step::Warning; break;
        case 1: m_step = Step::Legs; break;
        case 2: m_step = Step::Feet; break;
        default: m_step = Step::Check; break;
    }
}

std::string CalibrationSession::servosCommand() const
{
    return commandServoAt(kBaseGrade + m_trims[0], kBaseGrade + m_trims[1],
                          kBaseGrade + m_trims[2], kBaseGrade + m_trims[3]);
}

std::string CalibrationSession::trimsCommand() const
{
    return commandSetTrims(m_trims[0], m_trims[1], m_trims[2], m_trims[3]);
}

std::string CalibrationSession::neutralCommand() const
{
    return commandServoAt(kBaseGrade, kBaseGrade, kBaseGrade, kBaseGrade);
}

bool CalibrationSession::shouldSend(long long nowMs, long long &lastSendMs) const
{
    if (nowMs - lastSendMs < kDebounceMs) return false;
    lastSendMs = nowMs;
    return true;
}

} // namespace zowi