#pragma once

#include <QObject>
#include <QString>
#include <QTimer>
#include <QVector>
#include <memory>

#include <zowi/zowi_dice.h>

class RobotController;
class SessionController;
class CommandsController;

class ZowiDiceController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    Q_PROPERTY(int sequenceLength READ sequenceLength NOTIFY sequenceLengthChanged)
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int currentStep READ currentStep NOTIFY stepChanged)
    Q_PROPERTY(bool blockUserInput READ blockUserInput NOTIFY blockUserInputChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    // Named state constants: QML enum lookups like `ZowiDice.State.Idle` do
    // NOT resolve for context-property instances, so comparisons must use
    // these value properties instead (e.g. `ZowiDice.state === ZowiDice.stateIdle`).
    Q_PROPERTY(int stateIdle READ stateIdle CONSTANT)
    Q_PROPERTY(int stateShowingSequence READ stateShowingSequence CONSTANT)
    Q_PROPERTY(int stateWaitingForUser READ stateWaitingForUser CONSTANT)
    Q_PROPERTY(int stateGameOver READ stateGameOver CONSTANT)

public:
    enum State {
        Idle = 0,
        ShowingSequence = 1,
        WaitingForUser = 2,
        GameOver = 3
    };
    Q_ENUM(State)

    enum Action {
        TiptoeSwing = 0,
        BendBackward = 1,
        Jump = 2,
        MoonwalkerRight = 3
    };
    Q_ENUM(Action)

    explicit ZowiDiceController(QObject* parent = nullptr);
    ~ZowiDiceController();

    void setRobotController(RobotController* robot);
    void setSessionController(SessionController* session);
    void setCommandsController(CommandsController* commands);

    int state() const;
    int score() const;
    int sequenceLength() const;
    int progress() const;
    int currentStep() const;
    bool blockUserInput() const;
    bool connected() const;
    int stateIdle() const { return static_cast<int>(State::Idle); }
    int stateShowingSequence() const { return static_cast<int>(State::ShowingSequence); }
    int stateWaitingForUser() const { return static_cast<int>(State::WaitingForUser); }
    int stateGameOver() const { return static_cast<int>(State::GameOver); }

    Q_INVOKABLE void startGame();
    Q_INVOKABLE void onActionTopLeft();
    Q_INVOKABLE void onActionTopRight();
    Q_INVOKABLE void onActionBottomLeft();
    Q_INVOKABLE void onActionBottomRight();
    Q_INVOKABLE void resetGame();

signals:
    void stateChanged();
    void scoreChanged();
    void sequenceLengthChanged();
    void progressChanged();
    void stepChanged();
    void blockUserInputChanged();
    void connectedChanged();
    void gameOver(int score);
    void sendCommand(const QString& cmd);

private:
    void handleUserAction(zowi::ZowiDiceAction action);
    void onRobotSoftwareAck();
    void onRobotFinalAck();
    void sendNextRobotCommand();
    void sendGameOverGesture();
    void updateFromGame();
    void saveLastScore();

    std::unique_ptr<zowi::ZowiDiceGame> m_game;
    RobotController* m_robot = nullptr;
    SessionController* m_session = nullptr;
    CommandsController* m_commands = nullptr;
    bool m_connected = false;
    // Safety net: if the robot never acknowledges a movement (no &&A), the
    // game would wait forever. Mirrors the CLI's 20 s start timeout; on fire it
    // stops the robot and returns the game to Idle.
    QTimer m_moveStartTimeout;
};
