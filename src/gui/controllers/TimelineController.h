#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <memory>

#include <vector>

#include <zowi/timeline_command.h>
#include <zowi/timeline_player.h>

class SessionController;
class RobotController;
class CommandsController;

class TimelineController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(int currentChipIndex READ currentChipIndex NOTIFY currentChipIndexChanged)

public:
    explicit TimelineController(QObject* parent = nullptr);
    ~TimelineController();

    void setSessionController(SessionController* session);
    void setRobotController(RobotController* robot);
    void setCommandsController(CommandsController* commands);

    bool isPlaying() const;
    int currentIndex() const;
    int currentChipIndex() const;

    Q_INVOKABLE bool commandSupportsDirection(const QString& name) const;
    Q_INVOKABLE bool commandUsesFrontBackDirection(const QString& name) const;

    Q_INVOKABLE void saveSequence(const QVariantList &items);
    Q_INVOKABLE QVariantList loadSequence() const;
    Q_INVOKABLE void play(const QVariantList &items);
    Q_INVOKABLE void stop();

signals:
    void isPlayingChanged();
    void currentIndexChanged();
    // Emitted once a whole sequence ran to completion with real robot acks
    // (never after Stop or a timeout). `eligible` is false when the sequence
    // is too short for the ranking; the score itself comes from timelineScore().
    void sequenceCompleted(int score, bool eligible);
    void currentChipIndexChanged();

private slots:
    void onRobotSoftwareAck();
    void onRobotFinalAck();
    void sendNextCommand();
    void onMotionlessDisplayTimeout();

private:
    SessionController* m_session = nullptr;
    RobotController* m_robot = nullptr;
    CommandsController* m_commands = nullptr;

    std::unique_ptr<zowi::TimelinePlayer> m_player;
    std::vector<zowi::TimelineCommand> m_playedCommands;  // steps actually sent, for scoring
    QTimer m_moveStartTimeout;     // Safety net for missing &&A
    QTimer m_motionlessDisplay;    // Display time for animations/mouths

    int getDurationMs(const QString& duration) const;
    QString commandToString(const QVariantMap& cmd);
    static zowi::TimelineCommand commandFromMap(const QVariantMap& map);
    void updateFromPlayer();
};
