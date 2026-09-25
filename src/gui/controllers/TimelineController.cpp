#include "TimelineController.h"
#include "SessionController.h"
#include "RobotController.h"
#include "CommandsController.h"
#include "zowi/timeline_command.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

TimelineController::TimelineController(QObject* parent)
    : QObject(parent)
{
    connect(&m_playbackTimer, &QTimer::timeout, this, &TimelineController::playNext);
}

TimelineController::~TimelineController() = default;

void TimelineController::setSessionController(SessionController* session) {
    m_session = session;
}

void TimelineController::setRobotController(RobotController* robot) {
    m_robot = robot;
}

void TimelineController::setCommandsController(CommandsController* commands) {
    m_commands = commands;
}

bool TimelineController::isPlaying() const {
    return m_isPlaying;
}

int TimelineController::currentIndex() const {
    return m_currentIndex;
}

void TimelineController::saveSequence(const QVariantList &items) {
    if (!m_session) return;

    std::vector<zowi::TimelineCommand> timeline;
    for (const auto& item : items) {
        auto map = item.toMap();

        zowi::TimelineItemType type = zowi::TimelineItemType::Movement;
        QString typeStr = map.value("type", "movement").toString();
        if (typeStr == "animation") {
            type = zowi::TimelineItemType::Animation;
        } else if (typeStr == "mouth") {
            type = zowi::TimelineItemType::Mouth;
        }

        zowi::TimelineDuration duration = zowi::TimelineDuration::Medium;
        QString durationStr = map.value("duration", "Medium").toString();
        if (durationStr == "Slow") {
            duration = zowi::TimelineDuration::Slow;
        } else if (durationStr == "Fast") {
            duration = zowi::TimelineDuration::Fast;
        }

        zowi::TimelineDirection direction = zowi::TimelineDirection::Front;
        QString directionStr = map.value("direction", "Front").toString();
        if (directionStr == "Left") {
            direction = zowi::TimelineDirection::Left;
        } else if (directionStr == "Right") {
            direction = zowi::TimelineDirection::Right;
        }

        timeline.emplace_back(
            type,
            map.value("name", "").toString().toStdString(),
            map.value("reps", 1).toInt(),
            duration,
            direction
        );
    }

    std::string json = zowi::serializeTimeline(timeline);
    m_session->saveString("timeline_sequence", QString::fromStdString(json));
    qDebug() << "[Timeline] Saved" << items.size() << "commands to timeline_sequence";
}

QVariantList TimelineController::loadSequence() const {
    QVariantList result;
    if (!m_session) return result;

    QString jsonStr = m_session->getString("timeline_sequence", "[]");
    std::vector<zowi::TimelineCommand> timeline = zowi::parseTimeline(jsonStr.toStdString());

    for (const auto& cmd : timeline) {
        QVariantMap map;
        switch (cmd.type) {
            case zowi::TimelineItemType::Movement:
                map["type"] = "movement";
                break;
            case zowi::TimelineItemType::Animation:
                map["type"] = "animation";
                break;
            case zowi::TimelineItemType::Mouth:
                map["type"] = "mouth";
                break;
        }

        map["name"] = QString::fromStdString(cmd.name);
        map["reps"] = cmd.repetitions;

        switch (cmd.duration) {
            case zowi::TimelineDuration::Slow:
                map["duration"] = "Slow";
                break;
            case zowi::TimelineDuration::Fast:
                map["duration"] = "Fast";
                break;
            default:
                map["duration"] = "Medium";
                break;
        }

        switch (cmd.direction) {
            case zowi::TimelineDirection::Left:
                map["direction"] = "Left";
                break;
            case zowi::TimelineDirection::Right:
                map["direction"] = "Right";
                break;
            default:
                map["direction"] = "Front";
                break;
        }

        // Compute supportsDuration / supportsDirection (mirrors GameTimelineScreen logic)
        if (cmd.type == zowi::TimelineItemType::Movement) {
            map["supportsDuration"] = true;
            // Only Crusaito supports direction
            map["supportsDirection"] = (cmd.name == "Crusaito");
        } else {
            map["supportsDuration"] = false;
            map["supportsDirection"] = false;
        }

        result.append(map);
    }

    qDebug() << "[Timeline] Loaded" << result.size() << "commands from timeline_sequence";
    return result;
}

int TimelineController::getDurationMs(const QString& duration) const {
    if (duration == "Slow") return 2000;
    if (duration == "Fast") return 700;
    return 1000; // Medium (default)
}

QString TimelineController::commandToString(const QVariantMap& cmd) {
    if (!m_commands || !m_robot) return QString();

    QString type = cmd.value("type", "movement").toString();
    QString name = cmd.value("name", "").toString();
    int duration = getDurationMs(cmd.value("duration", "Medium").toString());

    if (type == "movement") {
        if (name == "Walk Forward") return m_commands->walkForward(duration);
        if (name == "Walk Backward") return m_commands->walkBackward(duration);
        if (name == "Turn Left") return m_commands->turnLeft(duration);
        if (name == "Turn Right") return m_commands->turnRight(duration);
        if (name == "Moonwalker Left") return m_commands->moonwalkerLeft(duration);
        if (name == "Moonwalker Right") return m_commands->moonwalkerRight(duration);
        if (name == "Bend Forward") return m_commands->bendForward(duration);
        if (name == "Shake Leg") return m_commands->shakeLegLeft(duration);
        if (name == "Up/Down") return m_commands->updown(duration);
        if (name == "Jitter") return m_commands->jitter(duration);
        if (name == "Swing") return m_commands->swing(duration);
        if (name == "Flapping") return m_commands->flappingLeft(duration);
        if (name == "Crusaito") {
            QString dir = cmd.value("direction", "Front").toString();
            return dir == "Left" ? m_commands->crusaitoBackward(duration)
                                 : m_commands->crusaitoForward(duration);
        }
    } else if (type == "animation") {
        // Map animation names to gesture IDs
        if (name == "Happy") return m_commands->gestureById(0);
        if (name == "SuperHappy") return m_commands->gestureById(1);
        if (name == "Sad") return m_commands->gestureById(2);
        if (name == "Sleeping") return m_commands->gestureById(3);
        if (name == "Fart") return m_commands->gestureById(4);
        if (name == "Confused") return m_commands->gestureById(5);
        if (name == "Love") return m_commands->gestureById(6);
        if (name == "Angry") return m_commands->gestureById(7);
        if (name == "Fretful") return m_commands->gestureById(8);
        if (name == "Magic") return m_commands->gestureById(9);
        if (name == "Wave") return m_commands->gestureById(10);
        if (name == "Victory") return m_commands->gestureById(11);
        if (name == "Fail") return m_commands->gestureById(12);
    } else if (type == "mouth") {
        // Map mouth names to mouth IDs
        if (name == "Smile") return m_commands->mouthById(0);
        if (name == "HappyOpen") return m_commands->mouthById(1);
        if (name == "HappyClosed") return m_commands->mouthById(2);
        if (name == "Heart") return m_commands->mouthById(3);
        if (name == "BigSurprise") return m_commands->mouthById(4);
        if (name == "SmallSurprise") return m_commands->mouthById(5);
        if (name == "TongueOut") return m_commands->mouthById(6);
        if (name == "Vamp1") return m_commands->mouthById(7);
        if (name == "Vamp2") return m_commands->mouthById(8);
        if (name == "LineMouth") return m_commands->mouthById(9);
        if (name == "Confused") return m_commands->mouthById(10);
        if (name == "DiagLeft") return m_commands->mouthById(11);
        if (name == "Sad") return m_commands->mouthById(12);
        if (name == "SadOpen") return m_commands->mouthById(13);
        if (name == "SadClosed") return m_commands->mouthById(14);
        if (name == "Ok") return m_commands->mouthById(15);
        if (name == "X") return m_commands->mouthById(16);
        if (name == "Interrogation") return m_commands->mouthById(17);
        if (name == "Thunder") return m_commands->mouthById(18);
        if (name == "Culito") return m_commands->mouthById(19);
        if (name == "Angry") return m_commands->mouthById(20);
    }
    return QString();
}

void TimelineController::play(const QVariantList &items) {
    if (!m_robot || !m_robot->isConnected()) {
        qWarning() << "[Timeline] Cannot play: robot not connected";
        return;
    }

    m_currentSequence = items;
    m_currentIndex = 0;
    m_isPlaying = true;
    emit isPlayingChanged();

    playNext();
}

void TimelineController::stop() {
    m_playbackTimer.stop();
    m_isPlaying = false;
    m_currentIndex = -1;
    emit isPlayingChanged();
    emit currentIndexChanged();

    if (m_robot) {
        m_robot->sendData(m_commands->stop());
        qDebug() << "[Timeline] Playback stopped";
    }
}

void TimelineController::playNext() {
    if (m_currentIndex >= m_currentSequence.length()) {
        stop();
        return;
    }

    QVariantMap cmd = m_currentSequence.at(m_currentIndex).toMap();
    int reps = cmd.value("reps", 1).toInt();
    int duration = getDurationMs(cmd.value("duration", "Medium").toString());

    for (int i = 0; i < reps; ++i) {
        QString cmdStr = commandToString(cmd);
        if (!cmdStr.isEmpty() && m_robot) {
            m_robot->sendData(cmdStr);
        }
    }

    emit currentIndexChanged();
    m_currentIndex++;

    // Schedule next command
    m_playbackTimer.start(duration);
}
