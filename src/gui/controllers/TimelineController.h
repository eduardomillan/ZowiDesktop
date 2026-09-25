#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QTimer>

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
    void playNext();

private:
    SessionController* m_session = nullptr;
    RobotController* m_robot = nullptr;
    CommandsController* m_commands = nullptr;
    bool m_isPlaying = false;
    int m_currentIndex = -1;
    QVariantList m_currentSequence;
    QTimer m_playbackTimer;

    int getDurationMs(const QString& duration) const;
    QString commandToString(const QVariantMap& cmd);
};
