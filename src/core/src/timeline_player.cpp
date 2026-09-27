#include "zowi/timeline_player.h"
#include "zowi/protocol.h"

namespace zowi {

void TimelinePlayer::start(const std::vector<TimelineStep>& steps) {
    m_steps = steps;
    m_currentStepIndex = 0;
    m_phase = StepPhase::Idle;
    m_stopQueued = false;
    m_stopAckSeen = false;
    m_motionlessRepsRemaining = 0;
    m_moveSeq.reset();
    m_state = TimelinePlayerState::Playing;
}

void TimelinePlayer::cancel() {
    m_moveSeq.reset();
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
        // Movement step: drive via MovementSequencer
        if (m_phase == StepPhase::Idle) {
            // Arm the movement: start the sequencer with the chip's reps as cycles
            m_moveSeq.start(step.cycles, step.speed);
            m_phase = StepPhase::MoveQueued;
            m_currentSpeed = step.speed;
            m_stopQueued = false;
            m_stopAckSeen = false;
            return step.wireCommand;  // Send the M command
        }
        if (m_phase == StepPhase::MoveRunning && !m_stopQueued && m_moveSeq.shouldQueueStop()) {
            // The movement's &&A just came in (cycle 1 running): send Stop NOW
            m_stopQueued = true;
            return commandStop();
        }
    } else {
        // Non-movement step (animation/mouth): send once, wait for ack + duration time
        if (m_phase == StepPhase::Idle) {
            m_phase = StepPhase::MotionlessRunning;
            m_currentSpeed = step.speed;  // Use speed as display duration
            m_stopAckSeen = false;
            return step.wireCommand;
        }
    }

    return {};
}

void TimelinePlayer::onSoftwareAck() {
    if (m_state != TimelinePlayerState::Playing) return;

    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return;

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement) {
        if (m_phase == StepPhase::MoveQueued) {
            // M command accepted: feed the ack to the sequencer
            RobotMessage ack;
            ack.cmd = toChar(Command::Ack);
            m_moveSeq.onMessage(ack);  // Transitions to Counting
            m_phase = StepPhase::MoveRunning;
        } else if (m_phase == StepPhase::AwaitingStopAcks) {
            // Stop command accepted (drain phase)
            m_stopAckSeen = true;
        }
    }
    // Note: non-movements don't wait for acks (firmware doesn't send them for H/L commands)
}

void TimelinePlayer::onFinalAck() {
    if (m_state != TimelinePlayerState::Playing) return;

    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) return;

    const TimelineStep& step = m_steps[m_currentStepIndex];

    if (step.isMovement) {
        if (m_phase == StepPhase::MoveRunning) {
            // One cycle completed
            RobotMessage fin;
            fin.cmd = toChar(Command::FinalAck);
            m_moveSeq.onMessage(fin);
            if (m_moveSeq.finished()) {
                m_phase = StepPhase::AwaitingStopAcks;
            }
        } else if (m_phase == StepPhase::AwaitingStopAcks && m_stopAckSeen) {
            // Stop fully done; move to next step
            advanceToNextStep();
        }
    } else {
        // Non-movement (gesture/mouth): firmware sends real &&F when done
        // Transition to awaiting-display phase; the UI timer will dispatch advancement
        if (m_phase == StepPhase::MotionlessRunning) {
            m_phase = StepPhase::MotionlessAwaitingDisplay;
        }
    }
}

void TimelinePlayer::advanceToNextStep() {
    m_currentStepIndex++;
    m_phase = StepPhase::Idle;
    m_stopQueued = false;
    m_stopAckSeen = false;
    m_motionlessRepsRemaining = 0;
    m_moveSeq.reset();

    if (m_currentStepIndex >= static_cast<int>(m_steps.size())) {
        // Sequence complete, but append a trailing Stop (safety net)
        m_state = TimelinePlayerState::Finished;
    }
}

} // namespace zowi
