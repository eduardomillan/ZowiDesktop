#include "zowi/timeline_player.h"
#include "zowi/protocol.h"

namespace zowi {

void TimelinePlayer::start(const std::vector<TimelineStep>& steps) {
    m_steps = steps;
    m_currentStepIndex = 0;
    m_phase = StepPhase::Idle;
    m_moveSeq.reset();
    m_stopSent = false;
    m_state = TimelinePlayerState::Playing;
}

void TimelinePlayer::cancel() {
    m_state = TimelinePlayerState::Idle;
    m_phase = StepPhase::Idle;
    m_moveSeq.reset();
    m_stopSent = false;
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
        if (m_phase == StepPhase::Idle) {
            // First call for this movement step: arm the sequencer and send M exactly once.
            m_phase = StepPhase::MoveActive;
            m_currentSpeed = step.speed;
            m_stopSent = false;
            m_moveSeq.reset();
            m_moveSeq.start(step.cycles, step.speed);
            return step.wireCommand;  // Send the M command
        }
        if (m_phase == StepPhase::MoveActive) {
            // Queue the Stop as soon as the sequencer asks for it: immediately
            // after &&A for cycles<=1, or after ack N-1 for cycles>1. Sent once.
            if (m_moveSeq.shouldQueueStop() && !m_stopSent) {
                m_stopSent = true;
                return commandStop();
            }
        }
        return {};  // waiting on &&A, a cycle &&F, or the drain
    } else {
        // Non-movement step (animation/mouth): send once, wait for ack + duration time
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

    if (step.isMovement && m_phase == StepPhase::MoveActive) {
        // Unconditional forward to sequencer; it handles its own state internally
        RobotMessage ack;
        ack.cmd = toChar(Command::Ack);
        m_moveSeq.onMessage(ack);
    }
}

void TimelinePlayer::onFinalAck() {
    if (m_state != TimelinePlayerState::Playing) return;
    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return;

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement && m_phase == StepPhase::MoveActive) {
        // Unconditional forward to sequencer
        RobotMessage fin;
        fin.cmd = toChar(Command::FinalAck);
        m_moveSeq.onMessage(fin);
        // Do NOT advance here. Advancement is now driven by the controller
        // via movementAwaitingAdvance()/advanceMovement() after the stop-ack drain.
    } else if (!step.isMovement && m_phase == StepPhase::MotionlessRunning) {
        // Non-movement: firmware sent &&F when done; wait for display timer
        m_phase = StepPhase::MotionlessAwaitingDisplay;
    }
}

void TimelinePlayer::advanceToNextStep() {
    m_currentStepIndex++;
    m_phase = StepPhase::Idle;
    m_moveSeq.reset();
    m_stopSent = false;

    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) {
        m_state = TimelinePlayerState::Finished;
    }
}

bool TimelinePlayer::movementAwaitingAdvance() const {
    return isPlaying() && currentStepIsMovement()
        && m_phase == StepPhase::MoveActive && m_moveSeq.finished();
}

void TimelinePlayer::advanceMovement() {
    if (!movementAwaitingAdvance()) return;
    advanceToNextStep();
}

void TimelinePlayer::advanceNonmovement() {
    // Display timer expired; move to next step
    advanceToNextStep();
}

} // namespace zowi
