#pragma once

#include <QObject>
#include <QString>
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

public:
    enum State {
        Idle = 0,
        ShowingSequence = 1,
        WaitingForUser = 2,
        GameOver = 3
    };
    Q_ENUM(State)

    enum Action {
        WalkForward = 0,
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
};
