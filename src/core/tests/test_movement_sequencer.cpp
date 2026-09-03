#include "zowi/movement_sequencer.h"
#include "zowi/message_parser.h"

#include <iostream>
#include <string>
#include <vector>

using namespace zowi;

static int failures = 0;

static void checkBool(bool actual, bool expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << (actual ? "true" : "false")
                  << " expected " << (expected ? "true" : "false") << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkInt(int actual, int expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << actual << " expected " << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

// Feeds a raw robot stream through a MessageParser and into the sequencer,
// exactly like a driver would.
static void feed(MessageParser &p, MovementSequencer &seq, const std::string &chunk)
{
    p.feed(chunk);
    for (const auto &msg : p.drain())
        seq.onMessage(msg);
}

int main()
{
    // ── Timeout guidance ───────────────────────────────────────────────────
    {
        MovementSequencer seq;
        seq.start(3, MovementSpeed::Slow);
        checkInt(seq.cycleTimeoutMs(), 3500, "cycle timeout slow (2000+1500)");
        seq.start(2, MovementSpeed::Fast);
        checkInt(seq.cycleTimeoutMs(), 2200, "cycle timeout fast (700+1500)");
        checkInt(seq.startTimeoutMs(), 20000, "start timeout");
        checkBool(seq.started(), false, "not started before &&A");
    }

    // ── Three-cycle movement (the verified hardware sequence) ─────────────
    {
        MessageParser p;
        MovementSequencer seq;
        int started = 0, stopQueued = 0, completed = 0;
        int lastCycle = 0, finishedCount = 0;
        seq.onStarted = [&] { ++started; };
        seq.onStopQueued = [&] { ++stopQueued; };
        seq.onCycleCompleted = [&](int k) { ++completed; lastCycle = k; };
        seq.onCompleted = [&] { ++finishedCount; };
        seq.start(3, MovementSpeed::Medium);

        // Stale &&F before the movement's &&A must be ignored.
        feed(p, seq, "&&F%%");
        checkBool(seq.started(), false, "stale &&F ignored before &&A");
        checkInt(seq.completedCycles(), 0, "no acks before &&A");

        // Backlog identity traffic is ignored too.
        feed(p, seq, "&&E Rebeconowi%%&&B 90.0%%");
        checkInt(seq.completedCycles(), 0, "identity traffic ignored");

        // The movement starts.
        feed(p, seq, "&&A%%");
        checkInt(started, 1, "started once on &&A");
        checkBool(seq.shouldQueueStop(), false, "no stop queued after 3-cycle start");

        // Cycle 1.
        feed(p, seq, "&&F%%");
        checkInt(lastCycle, 1, "cycle 1 completed");
        checkBool(seq.shouldQueueStop(), false, "no stop queued after ack 1/3");

        // Cycle 2 → the stop must be queued mid-cycle-3 (after ack N-1).
        feed(p, seq, "&&F%%");
        checkInt(lastCycle, 2, "cycle 2 completed");
        checkBool(seq.shouldQueueStop(), true, "stop queued after ack 2/3");
        checkInt(stopQueued, 1, "onStopQueued fired once");

        // Cycle 3 → done; the stop's own acks afterwards are ignored.
        feed(p, seq, "&&F%%");
        checkInt(lastCycle, 3, "cycle 3 completed");
        checkBool(seq.finished(), true, "finished after ack 3/3");
        checkInt(finishedCount, 1, "onCompleted fired once");
        feed(p, seq, "&&A%%&&F%%");
        checkInt(completed, 3, "stop acks ignored after finish");
        checkInt(finishedCount, 1, "onCompleted not re-fired");
    }

    // ── Single-cycle movement: stop queued immediately after &&A ──────────
    {
        MessageParser p;
        MovementSequencer seq;
        int stopQueued = 0, finishedCount = 0;
        seq.onStopQueued = [&] { ++stopQueued; };
        seq.onCompleted = [&] { ++finishedCount; };
        seq.start(1, MovementSpeed::Fast);

        feed(p, seq, "&&A%%");
        checkBool(seq.shouldQueueStop(), true, "stop queued right after start (1 cycle)");
        checkInt(stopQueued, 1, "onStopQueued fired for 1-cycle movement");
        checkBool(seq.finished(), false, "not finished before the cycle ack");

        feed(p, seq, "&&F%%");
        checkBool(seq.finished(), true, "finished after the single cycle");
        checkInt(finishedCount, 1, "onCompleted fired");
    }

    // ── Legacy lines and re-arm ────────────────────────────────────────────
    {
        MessageParser p;
        MovementSequencer seq;
        seq.start(2, MovementSpeed::Medium);
        feed(p, seq, "N Zowi\n");  // legacy name line: ignored
        checkInt(seq.completedCycles(), 0, "legacy lines ignored");

        // Reset returns to Idle and clears the latch.
        feed(p, seq, "&&A%%&&F%%");
        checkBool(seq.shouldQueueStop(), true, "stop queued for 2-cycle movement");
        seq.reset();
        checkBool(seq.started(), false, "reset clears started");
        checkBool(seq.shouldQueueStop(), false, "reset clears the stop latch");

        // Re-arm: the same stream runs a fresh movement.
        seq.start(2, MovementSpeed::Medium);
        feed(p, seq, "&&A%%&&F%%&&F%%");
        checkBool(seq.finished(), true, "re-armed movement completes");
    }

    if (failures == 0) {
        std::cout << "All movement_sequencer tests passed.\n";
        return 0;
    }
    std::cerr << failures << " movement_sequencer test(s) failed.\n";
    return 1;
}
