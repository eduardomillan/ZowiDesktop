#pragma once

#include <vector>
#include <string>

#include <zowi/movement_sequencer.h>

namespace zowi {

enum class ZowiDiceAction {
    TiptoeSwing,
    BendBackward,
    Jump,
    MoonwalkerRight
};

enum class ZowiDiceState {
    Idle,
    ShowingSequence,
    WaitingForUser,
    GameOver
};

struct ZowiDiceConfig {
    int initialSpeedMs = 1000;
    int achievementThreshold = 12;
    int rankThreshold = 3;
};

class ZowiDiceGame {
public:
    explicit ZowiDiceGame(const ZowiDiceConfig& cfg = {});
    ~ZowiDiceGame() = default;

    void startGame();
    void onUserAction(ZowiDiceAction action);
    // Robot ack hooks, fed by the controller (mirrors the CLI's ack plumbing):
    //   onSoftwareAck()  — robot sent &&A%% (command accepted, not processed)
    //   onFinalAck()     — robot sent &&F%% (command fully processed / one gait
    //                      cycle completed)
    void onSoftwareAck();
    void onFinalAck();
    void reset();

    ZowiDiceState state() const { return m_state; }
    int currentScore() const { return m_score; }
    int sequenceLength() const { return static_cast<int>(m_zowiSequence.size()); }
    const std::vector<ZowiDiceAction>& zowiSequence() const { return m_zowiSequence; }
    const std::vector<ZowiDiceAction>& userSequence() const { return m_userSequence; }
    std::string nextRobotCommand();
    bool shouldBlockUserInput() const;
    int progressPercent() const;
    // 0-based step currently in progress: during replay the index of the move
    // Zowi is executing; during the user's turn the number of moves repeated so
    // far. Used for the "X / Y" progress readout.
    int currentStep() const;

private:
    void addRandomAction();
    bool validateUserAction();
    void advanceToNextZowiAction();
    void checkGameOver();
    std::string buildCommandForAction(ZowiDiceAction action) const;
    void resetActionMachine();

    ZowiDiceConfig m_config;
    ZowiDiceState m_state = ZowiDiceState::Idle;
    std::vector<ZowiDiceAction> m_zowiSequence;
    std::vector<ZowiDiceAction> m_userSequence;
    int m_zowiSequenceIndex = 0;
    int m_score = 0;

    // Per-action ACK machine (only meaningful while ShowingSequence). The
    // firmware repeats the last `M` for one gait cycle per loop pass and only
    // reads serial between cycles, so a single move must be stopped DURING its
    // first cycle. Per action: M → &&A → S → &&F(move) → &&A(stop) → &&F(stop).
    enum class ActionPhase { Idle, MoveQueued, MoveRunning, AwaitingStopAcks };
    ActionPhase m_phase = ActionPhase::Idle;
    bool m_stopQueued = false;  // the Stop has been handed to the driver
    bool m_stopAckSeen = false; // the Stop's own &&A received (drain)
    MovementSequencer m_moveSeq;
};

} // namespace zowi