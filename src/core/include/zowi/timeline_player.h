#pragma once

#include <vector>
#include <string>

#include <zowi/robot_commands.h>

namespace zowi {

struct TimelineStep {
    std::string wireCommand;      // The wire string to send (M.../H.../L.../S...)
    bool isMovement;              // True if this is a movement
    MovementSpeed speed;          // Speed/duration in ms
};

enum class TimelinePlayerState {
    Idle,
    Playing,
    Finished
};

class TimelinePlayer {
public:
    TimelinePlayer() = default;
    ~TimelinePlayer() = default;

    void start(const std::vector<TimelineStep>& steps);
    void cancel();
    void reset();

    // Feed one parsed robot message from RobotController
    void onSoftwareAck();
    void onFinalAck();

    // Query next command to send
    std::string nextRobotCommand();

    TimelinePlayerState state() const { return m_state; }
    bool isPlaying() const { return m_state == TimelinePlayerState::Playing; }
    bool finished() const { return m_state == TimelinePlayerState::Finished; }
    int currentIndex() const { return m_currentStepIndex; }
    int totalSteps() const { return static_cast<int>(m_steps.size()); }
    int currentSpeed() const { return static_cast<int>(m_currentSpeed); }
    bool currentStepIsMovement() const {
        if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return false;
        return m_steps[m_currentStepIndex].isMovement;
    }

    // For UI-driven advancement of non-movement items after display timer
    void advanceNonmovement();

    // Timeout guidance
    int startTimeoutMs() const { return 20000; }
    int cycleTimeoutMs() const { return static_cast<int>(m_currentSpeed) + 1500; }

private:
    enum class StepPhase { Idle, MoveQueued, MoveRunning, MotionlessRunning, MotionlessAwaitingDisplay };

    void advanceToNextStep();

    TimelinePlayerState m_state = TimelinePlayerState::Idle;
    std::vector<TimelineStep> m_steps;
    int m_currentStepIndex = 0;
    StepPhase m_phase = StepPhase::Idle;
    bool m_softwareAckSeen = false;

    MovementSpeed m_currentSpeed = MovementSpeed::Medium;
};

} // namespace zowi
