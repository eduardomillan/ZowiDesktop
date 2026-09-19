#include "zowi/zowi_dice.h"
#include "zowi/robot_commands.h"
#include <random>

namespace zowi {

namespace {
std::mt19937& rng() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    return gen;
}

ZowiDiceAction randomAction() {
    std::uniform_int_distribution<int> dist(0, 3);
    return static_cast<ZowiDiceAction>(dist(rng()));
}
} // namespace

ZowiDiceGame::ZowiDiceGame(const ZowiDiceConfig& cfg)
    : m_config(cfg) {}

void ZowiDiceGame::startGame() {
    m_zowiSequence.clear();
    m_userSequence.clear();
    m_zowiSequenceIndex = 0;
    m_score = 0;
    m_waitingForAck = false;
    m_needToSendStop = false;
    m_state = ZowiDiceState::ShowingSequence;
    addRandomAction();
}

void ZowiDiceGame::reset() {
    m_zowiSequence.clear();
    m_userSequence.clear();
    m_zowiSequenceIndex = 0;
    m_score = 0;
    m_waitingForAck = false;
    m_needToSendStop = false;
    m_state = ZowiDiceState::Idle;
}

void ZowiDiceGame::addRandomAction() {
    m_zowiSequence.push_back(randomAction());
}

bool ZowiDiceGame::validateUserAction() {
    if (m_userSequence.empty()) return true;
    size_t idx = m_userSequence.size() - 1;
    if (idx >= m_zowiSequence.size()) return false;
    return m_userSequence[idx] == m_zowiSequence[idx];
}

void ZowiDiceGame::advanceToNextZowiAction() {
    m_zowiSequenceIndex++;
    if (m_zowiSequenceIndex >= static_cast<int>(m_zowiSequence.size())) {
        m_state = ZowiDiceState::WaitingForUser;
        m_zowiSequenceIndex = 0;
        m_waitingForAck = false;
    } else {
        m_waitingForAck = false;
    }
}

void ZowiDiceGame::checkGameOver() {
    if (!validateUserAction() || m_userSequence.size() > m_zowiSequence.size()) {
        m_state = ZowiDiceState::GameOver;
        m_score = static_cast<int>(m_zowiSequence.size()) - 1;
    } else if (m_userSequence.size() == m_zowiSequence.size()) {
        // Successful round: add new action, then score = new length - 1
        addRandomAction();
        m_score = static_cast<int>(m_zowiSequence.size()) - 1;
        m_userSequence.clear();
        m_zowiSequenceIndex = 0;
        m_state = ZowiDiceState::ShowingSequence;
        m_waitingForAck = false;
        m_needToSendStop = false;
    }
}

std::string ZowiDiceGame::buildCommandForAction(ZowiDiceAction action) const {
    MovementSpeed speed = static_cast<MovementSpeed>(m_config.initialSpeedMs);
    switch (action) {
        case ZowiDiceAction::WalkForward:
            return commandWalkForward(speed);
        case ZowiDiceAction::BendBackward:
            return commandBendBackward(speed);
        case ZowiDiceAction::Jump:
            return commandJump(speed);
        case ZowiDiceAction::MoonwalkerRight:
            return commandMoonwalkerRight(speed);
    }
    return {};
}

std::string ZowiDiceGame::nextRobotCommand() {
    if (m_state == ZowiDiceState::ShowingSequence) {
        if (!m_waitingForAck) {
            m_waitingForAck = true;
            m_needToSendStop = true;
            return buildCommandForAction(m_zowiSequence[m_zowiSequenceIndex]);
        } else if (m_needToSendStop) {
            m_needToSendStop = false;
            return commandStop();
        }
    }
    return {};
}

void ZowiDiceGame::onFinalAck() {
    if (m_state != ZowiDiceState::ShowingSequence) return;
    // A FinalAck arrives after each command (movement AND stop). Only advance
    // to the next action once the Stop command has been acknowledged; after a
    // movement ACK we still have to send the Stop for the current action.
    if (m_needToSendStop) return;
    advanceToNextZowiAction();
}

void ZowiDiceGame::onUserAction(ZowiDiceAction action) {
    if (m_state != ZowiDiceState::WaitingForUser) return;
    m_userSequence.push_back(action);
    checkGameOver();
}

bool ZowiDiceGame::shouldBlockUserInput() const {
    return m_state == ZowiDiceState::ShowingSequence;
}

int ZowiDiceGame::progressPercent() const {
    if (m_zowiSequence.empty()) return 0;
    if (m_state == ZowiDiceState::ShowingSequence) {
        return (m_zowiSequenceIndex * 100) / static_cast<int>(m_zowiSequence.size());
    }
    if (m_state == ZowiDiceState::WaitingForUser) {
        return (static_cast<int>(m_userSequence.size()) * 100) / static_cast<int>(m_zowiSequence.size());
    }
    return 0;
}

int ZowiDiceGame::currentStep() const {
    if (m_state == ZowiDiceState::ShowingSequence) return m_zowiSequenceIndex;
    if (m_state == ZowiDiceState::WaitingForUser) return static_cast<int>(m_userSequence.size());
    return 0;
}

} // namespace zowi