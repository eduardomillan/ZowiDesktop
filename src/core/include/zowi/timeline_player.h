#pragma once

#include <vector>
#include <string>

#include <zowi/movement_sequencer.h>
#include <zowi/robot_commands.h>

namespace zowi {

struct TimelineStep {
    std::string wireCommand;      // The wire string to send (M.../H.../L.../S...)
    bool isMovement;              // True if this is a movement (needs sequencer)
    int cycles;                   // How many cycles (reps) to execute (only for movements)
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

    // Feed one parsed robot message from RobotController::softwareAckReceived/finalAckReceived
    void onSoftwareAck();
    void onFinalAck();

    // Query next command to send (called by TimelineController after onSoftwareAck/onFinalAck)
    std::string nextRobotCommand();

    TimelinePlayerState state() const { return m_state; }
    bool isPlaying() const { return m_state == TimelinePlayerState::Playing; }
    bool finished() const { return m_state == TimelinePlayerState::Finished; }
    int currentIndex() const { return m_currentStepIndex; }
    int totalSteps() const { return static_cast<int>(m_steps.size()); }

    // Timeout guidance for drivers
    int startTimeoutMs() const { return 20000; }
    int cycleTimeoutMs() const { return static_cast<int>(m_currentSpeed) + 1500; }

private:
    enum class StepPhase { Idle, MoveQueued, MoveRunning, AwaitingStopAcks, MotionlessRunning };

    void advanceToNextStep();
    bool isMovementAtIndex(int idx) const;
    int cyclesForStep(int idx) const;
    MovementSpeed speedForStep(int idx) const;

    TimelinePlayerState m_state = TimelinePlayerState::Idle;
    std::vector<TimelineStep> m_steps;
    int m_currentStepIndex = 0;
    StepPhase m_phase = StepPhase::Idle;

    // Movement sequencer (reusable per movement step)
    MovementSequencer m_moveSeq;
    bool m_stopQueued = false;
    bool m_stopAckSeen = false;

    // For non-movement items (animation/mouth): how many reps left
    int m_motionlessRepsRemaining = 0;

    // Current step speed for timeout calculation
    MovementSpeed m_currentSpeed = MovementSpeed::Medium;
};

} // namespace zowi
