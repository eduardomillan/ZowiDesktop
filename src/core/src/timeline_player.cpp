#include "zowi/timeline_player.h"
#include "zowi/protocol.h"

namespace zowi {

void TimelinePlayer::start(const std::vector<TimelineStep>& steps) {
    m_steps = steps;
    m_currentStepIndex = 0;
    m_phase = StepPhase::Idle;
    m_softwareAckSeen = false;
    m_state = TimelinePlayerState::Playing;
}

void TimelinePlayer::cancel() {
    m_state = TimelinePlayerState::Idle;
    m_phase = StepPhase::Idle;
}

void TimelinePlayer::reset() {
    cancel();
    m_steps.clear();
    m_currentStepIndex = 0;
}

std::string TimelinePlayer::nextRobotCommand() {
    if (m_state != TimelinePlayerState::Playing || m_currentStepIndex >= static_cast<int>(m_steps.size())) {
        return {};
    }

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement) {
        // Movement: send M, wait for &&A + &&F
        if (m_phase == StepPhase::Idle) {
            m_phase = StepPhase::MoveQueued;
            m_currentSpeed = step.speed;
            m_softwareAckSeen = false;
            return step.wireCommand;  // Send the M command
        }
    } else {
        // Non-movement (animation/mouth): send H/L, wait for &&F, then display timer
        if (m_phase == StepPhase::Idle) {
            m_phase = StepPhase::MotionlessRunning;
            m_currentSpeed = step.speed;  // Use speed as display duration
            return step.wireCommand;
        }
    }

    return {};
}

void TimelinePlayer::onSoftwareAck() {
    if (m_state != TimelinePlayerState::Playing) return;
    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return;

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement && m_phase == StepPhase::MoveQueued) {
        // M command accepted
        m_phase = StepPhase::MoveRunning;
        m_softwareAckSeen = true;
    }
}

void TimelinePlayer::onFinalAck() {
    if (m_state != TimelinePlayerState::Playing) return;
    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return;

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement && m_phase == StepPhase::MoveRunning) {
        // Movement complete; advance to next step
        advanceToNextStep();
    } else if (!step.isMovement && m_phase == StepPhase::MotionlessRunning) {
        // Non-movement: firmware ack received; wait for display timer
        m_phase = StepPhase::MotionlessAwaitingDisplay;
    }
}

void TimelinePlayer::advanceToNextStep() {
    m_currentStepIndex++;
    m_phase = StepPhase::Idle;
    m_softwareAckSeen = false;

    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) {
        m_state = TimelinePlayerState::Finished;
    }
}

void TimelinePlayer::advanceNonmovement() {
    // Display timer expired; move to next step
    advanceToNextStep();
}

} // namespace zowi
