#pragma once

#include <functional>

#include <zowi/message_parser.h>
#include <zowi/robot_commands.h>

namespace zowi {

// Movement sequencer: the shared brain behind "move N cycles and stop".
//
// The firmware's MODE-4 loop emits one &&F (final ack) per completed gait
// cycle and only reads serial between cycles (each move() blocks for a full
// period T), so running exactly N cycles and ending stopped requires:
//
//   1. waiting for the movement's &&A (software ack) — it fires when the
//      robot has actually processed the M, which may take seconds while the
//      connect-time identity polling drains its RX queue;
//   2. counting the per-cycle &&F acks;
//   3. queueing the stop (S) DURING the last cycle — right after ack N-1 —
//      because a stop sent after ack N races with the loop turnaround and
//      lets an unrequested extra cycle slip in.
//
// This class owns that sequencing as a pure state machine: feed it the
// parsed RobotMessages and act on its events / queries. It sends nothing
// itself — the driver (CLI blocking loop, GUI event loop) owns the
// transport, the timers and the actual `send()`.
//
// Messages that are not &&A/&&F (and legacy lines) are ignored, so it is
// safe to feed it everything that comes out of a MessageParser.
class MovementSequencer {
public:
    // Optional lifecycle callbacks (all null-safe). onStopQueued fires once;
    // drivers that prefer polling can use shouldQueueStop() instead.
    std::function<void()> onStarted = nullptr;            // &&A received, cycle 1 begins
    std::function<void(int)> onCycleCompleted = nullptr;  // k-th cycle done (1-based)
    std::function<void()> onStopQueued = nullptr;         // queue the S now
    std::function<void()> onCompleted = nullptr;          // all cycles done

    // Arms the sequencer for a new movement.
    void start(int cycles, MovementSpeed speed);

    // Back to Idle (on (re)connect / new movement).
    void reset();

    // Feed one parsed robot message.
    void onMessage(const RobotMessage &msg);

    // True once &&A has been received (cycle 1 is running on the robot).
    bool started() const { return m_state != State::Idle && m_state != State::WaitStart; }
    // True when all cycles are done.
    bool finished() const { return m_state == State::Finished; }
    // Latched: true from the moment the stop should be queued (after ack
    // N-1, or immediately after started() when cycles == 1). Stays true
    // until the next start()/reset().
    bool shouldQueueStop() const { return m_stopQueued; }
    int completedCycles() const { return m_acks; }
    int totalCycles() const { return m_cycles; }

    // Timeout guidance for drivers. The &&A wait is generous because the
    // robot may still be draining the identity-poll backlog (~500 ms per
    // queued request). The per-cycle timeout allows one period T plus slack.
    int startTimeoutMs() const { return 20000; }
    int cycleTimeoutMs() const { return static_cast<int>(m_speed) + 1500; }

private:
    enum class State { Idle, WaitStart, Counting, Finished };

    void latchStopQueued();

    State m_state = State::Idle;
    int m_cycles = 0;
    int m_acks = 0;
    bool m_stopQueued = false;
    MovementSpeed m_speed = MovementSpeed::Medium;
};

} // namespace zowi
