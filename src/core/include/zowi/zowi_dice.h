#pragma once

#include <vector>
#include <string>

namespace zowi {

enum class ZowiDiceAction {
    WalkForward,
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

private:
    void addRandomAction();
    bool validateUserAction();
    void advanceToNextZowiAction();
    void checkGameOver();
    std::string buildCommandForAction(ZowiDiceAction action) const;

    ZowiDiceConfig m_config;
    ZowiDiceState m_state = ZowiDiceState::Idle;
    std::vector<ZowiDiceAction> m_zowiSequence;
    std::vector<ZowiDiceAction> m_userSequence;
    int m_zowiSequenceIndex = 0;
    int m_score = 0;
    bool m_waitingForAck = false;
    bool m_needToSendStop = false;
};

} // namespace zowi