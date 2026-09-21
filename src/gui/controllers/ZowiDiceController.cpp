#include "ZowiDiceController.h"
#include "RobotController.h"
#include "SessionController.h"
#include "CommandsController.h"

#include <zowi/robot_commands.h>

#include <QDebug>
#include <QTimer>

// Matches zowi::MovementSequencer::startTimeoutMs() (the CLI driver uses the
// same value as its &&A wait timeout).
namespace {
constexpr int kMoveStartTimeoutMs = 20000;
} // namespace

ZowiDiceController::ZowiDiceController(QObject* parent)
    : QObject(parent)
    , m_game(std::make_unique<zowi::ZowiDiceGame>())
{
    m_moveStartTimeout.setSingleShot(true);
    m_moveStartTimeout.setInterval(kMoveStartTimeoutMs);
    connect(&m_moveStartTimeout, &QTimer::timeout, this, [this]() {
        qWarning() << "[ZowiDice] Movement &&A not received within"
                   << kMoveStartTimeoutMs << "ms; stopping the robot and resetting the game.";
        if (m_robot && m_robot->isConnected())
            m_robot->sendData(QString::fromStdString(zowi::commandStop()));
        resetGame();
    });
}

ZowiDiceController::~ZowiDiceController() = default;

void ZowiDiceController::setRobotController(RobotController* robot) {
    m_robot = robot;
    if (m_robot) {
        m_connected = m_robot->isConnected();
        connect(m_robot, &RobotController::connectionChanged, this, [this]() {
            m_connected = m_robot->isConnected();
            emit connectedChanged();
        });
        connect(m_robot, &RobotController::softwareAckReceived, this, &ZowiDiceController::onRobotSoftwareAck);
        connect(m_robot, &RobotController::finalAckReceived, this, &ZowiDiceController::onRobotFinalAck);
    }
}

void ZowiDiceController::setSessionController(SessionController* session) {
    m_session = session;
}

void ZowiDiceController::setCommandsController(CommandsController* commands) {
    m_commands = commands;
}

int ZowiDiceController::state() const {
    return static_cast<int>(m_game->state());
}

int ZowiDiceController::score() const {
    return m_game->currentScore();
}

int ZowiDiceController::sequenceLength() const {
    return m_game->sequenceLength();
}

int ZowiDiceController::progress() const {
    return m_game->progressPercent();
}

int ZowiDiceController::currentStep() const {
    return m_game->currentStep();
}

bool ZowiDiceController::blockUserInput() const {
    return m_game->shouldBlockUserInput();
}

bool ZowiDiceController::connected() const {
    return m_connected;
}

void ZowiDiceController::startGame() {
    m_game->startGame();
    m_moveStartTimeout.stop();
    updateFromGame();
    sendNextRobotCommand();
}

void ZowiDiceController::resetGame() {
    m_game->reset();
    m_moveStartTimeout.stop();
    updateFromGame();
}

void ZowiDiceController::onActionTopLeft() {
    handleUserAction(zowi::ZowiDiceAction::TiptoeSwing);
}

void ZowiDiceController::onActionTopRight() {
    handleUserAction(zowi::ZowiDiceAction::BendBackward);
}

void ZowiDiceController::onActionBottomLeft() {
    handleUserAction(zowi::ZowiDiceAction::Jump);
}

void ZowiDiceController::onActionBottomRight() {
    handleUserAction(zowi::ZowiDiceAction::MoonwalkerRight);
}

void ZowiDiceController::handleUserAction(zowi::ZowiDiceAction action) {
    if (m_game->state() == zowi::ZowiDiceState::WaitingForUser) {
        m_game->onUserAction(action);
        updateFromGame();
        if (m_game->state() == zowi::ZowiDiceState::GameOver) {
            sendGameOverGesture();
            saveLastScore();
            emit gameOver(m_game->currentScore());
        } else if (m_game->state() == zowi::ZowiDiceState::ShowingSequence) {
            sendNextRobotCommand();
        }
    }
}

void ZowiDiceController::onRobotSoftwareAck() {
    // The move just got accepted (&&A): the game now asks for the Stop so it
    // lands mid-cycle-1 and the movement runs exactly once.
    m_moveStartTimeout.stop();
    m_game->onSoftwareAck();
    updateFromGame();
    if (m_game->state() == zowi::ZowiDiceState::ShowingSequence) {
        sendNextRobotCommand();
    }
}

void ZowiDiceController::onRobotFinalAck() {
    m_game->onFinalAck();
    updateFromGame();
    if (m_game->state() == zowi::ZowiDiceState::ShowingSequence) {
        sendNextRobotCommand();
    }
}

void ZowiDiceController::sendNextRobotCommand() {
    std::string cmd = m_game->nextRobotCommand();
    if (!cmd.empty() && m_robot && m_robot->isConnected()) {
        QString qcmd = QString::fromStdString(cmd);
        m_robot->sendData(qcmd);
        qDebug() << "[ZowiDice] Sending:" << qcmd.trimmed();
        // Arm the guard waiting for the move's &&A (restarts on each move).
        if (!qcmd.isEmpty() && qcmd[0] == QLatin1Char('M'))
            m_moveStartTimeout.start();
    }
}

// The robot reacts with an angry gesture when the player loses a round,
// mirroring ZowiAppReborn (which plays the ANGRY animation on game over).
void ZowiDiceController::sendGameOverGesture() {
    if (!m_robot || !m_robot->isConnected()) return;
    const QString cmd = QString::fromStdString(zowi::commandGesture(zowi::GestureId::Angry));
    m_robot->sendData(cmd);
    qDebug() << "[ZowiDice] Game over gesture:" << cmd.trimmed();
}

void ZowiDiceController::updateFromGame() {
    emit stateChanged();
    emit scoreChanged();
    emit sequenceLengthChanged();
    emit progressChanged();
    emit stepChanged();
    emit blockUserInputChanged();
}

void ZowiDiceController::saveLastScore() {
    if (m_session) {
        m_session->saveString("zowi_says_last_score", QString::number(m_game->currentScore()));
    }
}
