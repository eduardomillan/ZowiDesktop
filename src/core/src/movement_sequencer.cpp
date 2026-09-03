#include "zowi/movement_sequencer.h"

#include <zowi/protocol.h>

namespace zowi {

void MovementSequencer::start(int cycles, MovementSpeed speed)
{
    m_cycles = cycles;
    m_speed = speed;
    m_state = State::WaitStart;
    m_acks = 0;
    m_stopQueued = false;
}

void MovementSequencer::reset()
{
    m_state = State::Idle;
    m_cycles = 0;
    m_acks = 0;
    m_stopQueued = false;
}

void MovementSequencer::latchStopQueued()
{
    if (m_stopQueued) return;
    m_stopQueued = true;
    if (onStopQueued) onStopQueued();
}

void MovementSequencer::onMessage(const RobotMessage &msg)
{
    if (m_state == State::Idle || m_state == State::Finished) return;
    if (msg.legacy) return;

    if (msg.cmd == toChar(Command::Ack)) {
        if (m_state == State::WaitStart) {
            m_state = State::Counting;
            if (onStarted) onStarted();
            // A single cycle: the stop goes out right away so it lands
            // mid-cycle-1 (the robot reads serial only between cycles).
            if (m_cycles <= 1) latchStopQueued();
        }
        return;
    }

    if (msg.cmd == toChar(Command::FinalAck)) {
        // &&F acks are only meaningful once the movement started; anything
        // before the &&A is stale traffic (e.g. a previous command's ack).
        if (m_state != State::Counting) return;

        ++m_acks;
        if (m_acks < m_cycles) {
            if (onCycleCompleted) onCycleCompleted(m_acks);
            // Last cycle just started: queue the stop so it lands mid-cycle
            // and the robot homes right after ack N.
            if (m_acks == m_cycles - 1) latchStopQueued();
        } else {
            if (onCycleCompleted) onCycleCompleted(m_acks);
            m_state = State::Finished;
            if (onCompleted) onCompleted();
        }
    }
}

} // namespace zowi
