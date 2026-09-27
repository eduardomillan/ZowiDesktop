#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>
#include <memory>

#include <zowi/timeline_player.h>

class SessionController;
class RobotController;
class CommandsController;

class TimelineController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)

public:
    explicit TimelineController(QObject* parent = nullptr);
    ~TimelineController();

    void setSessionController(SessionController* session);
    void setRobotController(RobotController* robot);
    void setCommandsController(CommandsController* commands);

    bool isPlaying() const;
    int currentIndex() const;

    Q_INVOKABLE void saveSequence(const QVariantList &items);
    Q_INVOKABLE QVariantList loadSequence() const;
    Q_INVOKABLE void play(const QVariantList &items);
    Q_INVOKABLE void stop();

signals:
    void isPlayingChanged();
    void currentIndexChanged();

private slots:
    void onRobotSoftwareAck();
    void onRobotFinalAck();
    void sendNextCommand();
    void onMotionlessDisplayTimeout();
    void onStopAckDrainTimeout();

private:
    SessionController* m_session = nullptr;
    RobotController* m_robot = nullptr;
    CommandsController* m_commands = nullptr;

    std::unique_ptr<zowi::TimelinePlayer> m_player;
    QTimer m_moveStartTimeout;     // Safety net for missing &&A
    QTimer m_motionlessDisplay;    // Display time for animations/mouths
    QTimer m_stopAckDrain;         // Bounded drain wait for Stop's trailing ack

    int getDurationMs(const QString& duration) const;
    QString commandToString(const QVariantMap& cmd);
    void updateFromPlayer();
};
