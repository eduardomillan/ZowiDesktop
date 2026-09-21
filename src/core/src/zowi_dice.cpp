#include "zowi/zowi_dice.h"
#include "zowi/robot_commands.h"
#include "zowi/protocol.h"
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
    resetActionMachine();
    m_moveSeq.reset();
    m_state = ZowiDiceState::ShowingSequence;
    addRandomAction();
}

void ZowiDiceGame::reset() {
    m_zowiSequence.clear();
    m_userSequence.clear();
    m_zowiSequenceIndex = 0;
    m_score = 0;
    resetActionMachine();
    m_moveSeq.reset();
    m_state = ZowiDiceState::Idle;
}

void ZowiDiceGame::resetActionMachine() {
    m_phase = ActionPhase::Idle;
    m_stopQueued = false;
    m_stopAckSeen = false;
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
    resetActionMachine();
    if (m_zowiSequenceIndex >= static_cast<int>(m_zowiSequence.size())) {
        m_state = ZowiDiceState::WaitingForUser;
        m_zowiSequenceIndex = 0;
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
        resetActionMachine();
        m_state = ZowiDiceState::ShowingSequence;
    }
}

std::string ZowiDiceGame::buildCommandForAction(ZowiDiceAction action) const {
    MovementSpeed speed = static_cast<MovementSpeed>(m_config.initialSpeedMs);
    switch (action) {
        case ZowiDiceAction::TiptoeSwing:
            return commandTiptoeSwing(speed);
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
    if (m_state != ZowiDiceState::ShowingSequence) return {};
    if (m_phase == ActionPhase::Idle) {
        // Arm the action: the driver sends the move, then we wait for its &&A.
        MovementSpeed speed = static_cast<MovementSpeed>(m_config.initialSpeedMs);
        m_moveSeq.start(1, speed);
        m_phase = ActionPhase::MoveQueued;
        return buildCommandForAction(m_zowiSequence[m_zowiSequenceIndex]);
    }
    if (m_phase == ActionPhase::MoveRunning && !m_stopQueued && m_moveSeq.shouldQueueStop()) {
        // The move's &&A just came in (cycle 1 is running): the Stop must go
        // out NOW so it lands mid-cycle-1 and the robot homes right after the
        // single cycle — the firmware only reads serial between cycles, so a
        // Stop sent after the move's &&F would let an extra cycle slip in.
        m_stopQueued = true;
        return commandStop();
    }
    return {};
}

void ZowiDiceGame::onSoftwareAck() {
    if (m_state != ZowiDiceState::ShowingSequence) return;
    switch (m_phase) {
        case ActionPhase::MoveQueued: {
            RobotMessage ack;
            ack.cmd = toChar(Command::Ack);
            m_moveSeq.onMessage(ack); // → Counting; latchStopQueued (cycles == 1)
            m_phase = ActionPhase::MoveRunning;
            break;
        }
        case ActionPhase::AwaitingStopAcks:
            // &&A of the Stop (drain phase) — the robot has accepted the Stop.
            m_stopAckSeen = true;
            break;
        default:
            // Idle / MoveRunning: no move armed or the &&A is stale (e.g. a
            // leftover from a previous command). Ignored.
            break;
    }
}

void ZowiDiceGame::onFinalAck() {
    if (m_state != ZowiDiceState::ShowingSequence) return;
    switch (m_phase) {
        case ActionPhase::MoveRunning: {
            RobotMessage fin;
            fin.cmd = toChar(Command::FinalAck);
            m_moveSeq.onMessage(fin); // cycle 1 done → Finished
            if (m_moveSeq.finished())
                m_phase = ActionPhase::AwaitingStopAcks;
            break;
        }
        case ActionPhase::AwaitingStopAcks:
            // &&F of the Stop: the robot is at rest again. Advance the round
            // only now, so the Stop's acks never leak into the next move.
            if (m_stopAckSeen)
                advanceToNextZowiAction();
            break;
        default:
            // Idle / MoveQueued: &&F before the move started is stale traffic
            // (e.g. the previous Stop's final ack). Ignored.
            break;
    }
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