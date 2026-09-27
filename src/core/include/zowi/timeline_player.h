#pragma once

#include <vector>
#include <string>

#include <zowi/movement_sequencer.h>
#include <zowi/robot_commands.h>

namespace zowi {

struct TimelineStep {
    std::string wireCommand;      // The wire string to send (M.../H.../L.../S...)
    bool isMovement;              // True if this is a movement
    MovementSpeed speed;          // Speed/duration in ms
    int cycles = 1;               // Movement steps only: gait cycles to run back-to-back via MovementSequencer
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

    // Movement with cycles: true once the Nth cycle is counted and Stop already sent,
    // waiting for stop-ack drain before advancing to the next step
    bool movementAwaitingAdvance() const;

    // Called once the stop-ack drain window elapses (or a stray ack arrives early)
    void advanceMovement();

    // For UI-driven advancement of non-movement items after display timer
    void advanceNonmovement();

    // Timeout guidance
    int startTimeoutMs() const { return 20000; }
    int cycleTimeoutMs() const { return static_cast<int>(m_currentSpeed) + 1500; }
    int stopAckDrainTimeoutMs() const { return 2000; }

private:
    enum class StepPhase { Idle, MoveActive, MotionlessRunning, MotionlessAwaitingDisplay };

    void advanceToNextStep();

    TimelinePlayerState m_state = TimelinePlayerState::Idle;
    std::vector<TimelineStep> m_steps;
    int m_currentStepIndex = 0;
    StepPhase m_phase = StepPhase::Idle;

    MovementSequencer m_moveSeq;
    bool m_stopSent = false;  // prevents re-sending Stop for the same step

    MovementSpeed m_currentSpeed = MovementSpeed::Medium;
};

} // namespace zowi
