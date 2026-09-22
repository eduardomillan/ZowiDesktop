#include "MouthsGameController.h"
#include "RobotController.h"
#include "SessionController.h"

#include <zowi/robot_commands.h>

#include <QDebug>

namespace {
// Intermission between a solved round (VICTORY + Stop) and the next round.
constexpr int kVictoryPauseMs = 1500;
// Countdown tick: 100 ms keeps the progress bar smooth while staying cheap.
constexpr int kCountdownTickMs = 100;
} // namespace

MouthsGameController::MouthsGameController(QObject* parent)
    : QObject(parent)
    , m_game(std::make_unique<zowi::MouthsGame>())
{
    m_countdown.setInterval(kCountdownTickMs);
    connect(&m_countdown, &QTimer::timeout, this, &MouthsGameController::onCountdownTick);

    m_victoryPause.setSingleShot(true);
    m_victoryPause.setInterval(kVictoryPauseMs);
    connect(&m_victoryPause, &QTimer::timeout, this, &MouthsGameController::onVictoryPauseElapsed);
}

MouthsGameController::~MouthsGameController() = default;

void MouthsGameController::setRobotController(RobotController* robot) {
    m_robot = robot;
}

void MouthsGameController::setSessionController(SessionController* session) {
    m_session = session;
}

int MouthsGameController::state() const {
    return static_cast<int>(m_game->state());
}

int MouthsGameController::level() const {
    return m_game->level();
}

int MouthsGameController::score() const {
    return m_game->score();
}

int MouthsGameController::target() const {
    return static_cast<int>(m_game->target());
}

int MouthsGameController::targetPattern() const {
    return static_cast<int>(m_game->targetPattern());
}

int MouthsGameController::countdownMs() const {
    return m_countdownMs;
}

int MouthsGameController::roundTimeMs() const {
    return m_roundTimeMs;
}

void MouthsGameController::startGame() {
    m_game->startGame();
    m_victoryPause.stop();
    updateFromGame();
    startRound();
}

void MouthsGameController::resetGame() {
    m_game->reset();
    m_countdown.stop();
    m_victoryPause.stop();
    m_countdownMs = 0;
    m_roundTimeMs = 0;
    updateFromGame();
}

bool MouthsGameController::submitDraw(unsigned long matrix) {
    if (!m_game->submitDraw(matrix))
        return false;
    // Round solved: freeze the countdown, celebrate, then advance after the
    // VICTORY intermission (mirrors Android's [VICTORY, Stop] on a match).
    m_countdown.stop();
    m_countdownMs = 0;
    sendVictory();
    updateFromGame();
    m_victoryPause.start();
    return true;
}

// ── Round plumbing ──────────────────────────────────────────────────────────

void MouthsGameController::startRound() {
    m_roundTimeMs = m_game->countdownMsForLevel(m_game->level());
    m_countdownMs = m_roundTimeMs;
    emit countdownMsChanged();
    sendTargetMouth();
    m_countdown.start();
}

void MouthsGameController::onCountdownTick() {
    m_countdownMs -= kCountdownTickMs;
    if (m_countdownMs <= 0) {
        m_countdownMs = 0;
        m_countdown.stop();
        m_game->onTimeout();
        sendAngryAndStop();
        saveLastScore();
        updateFromGame();
        emit gameOver(m_game->score());
    }
    emit countdownMsChanged();
}

void MouthsGameController::onVictoryPauseElapsed() {
    // RoundSolved → next round. Only reachable after a solved round.
    m_game->advanceLevel();
    updateFromGame();
    startRound();
}

// ── Robot cosmetics (no-ops when not connected) ────────────────────────────

void MouthsGameController::sendTargetMouth() {
    if (!m_robot || !m_robot->isConnected()) return;
    const QString cmd = QString::fromStdString(zowi::commandMouth(m_game->targetPattern()));
    m_robot->sendData(cmd);
    qDebug() << "[Mouths] Showing target mouth:" << cmd.trimmed();
}

void MouthsGameController::sendVictory() {
    if (!m_robot || !m_robot->isConnected()) return;
    const QString victory = QString::fromStdString(zowi::commandGesture(zowi::GestureId::Victory));
    m_robot->sendData(victory);
    const QString stop = QString::fromStdString(zowi::commandStop());
    m_robot->sendData(stop);
    qDebug() << "[Mouths] Round solved:" << victory.trimmed() << "+" << stop.trimmed();
}

void MouthsGameController::sendAngryAndStop() {
    if (!m_robot || !m_robot->isConnected()) return;
    const QString angry = QString::fromStdString(zowi::commandGesture(zowi::GestureId::Angry));
    m_robot->sendData(angry);
    const QString stop = QString::fromStdString(zowi::commandStop());
    m_robot->sendData(stop);
    qDebug() << "[Mouths] Game over:" << angry.trimmed() << "+" << stop.trimmed();
}

// ── State / persistence ─────────────────────────────────────────────────────

void MouthsGameController::updateFromGame() {
    // targetChanged covers target + targetPattern (both derive from the same
    // core state change).
    emit stateChanged();
    emit levelChanged();
    emit scoreChanged();
    emit targetChanged();
}

void MouthsGameController::saveLastScore() {
    if (m_session)
        m_session->saveString("mouths_last_score", QString::number(m_game->score()));
}