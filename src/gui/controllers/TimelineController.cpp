#include "TimelineController.h"
#include "SessionController.h"
#include "RobotController.h"
#include "CommandsController.h"
#include "zowi/timeline_command.h"
#include "zowi/robot_commands.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>

namespace {
constexpr int kMoveStartTimeoutMs = 20000;

bool supportsFrontBackDirection(const QString& name) {
    return name == "Crusaito" || name == "Bend Forward";
}

bool supportsLeftRightDirection(const QString& name) {
    return name == "Shake Leg" || name == "Flapping";
}
}

TimelineController::TimelineController(QObject* parent)
    : QObject(parent)
    , m_player(std::make_unique<zowi::TimelinePlayer>())
{
    m_moveStartTimeout.setSingleShot(true);
    m_moveStartTimeout.setInterval(kMoveStartTimeoutMs);
    connect(&m_moveStartTimeout, &QTimer::timeout, this, [this]() {
        qWarning() << "[Timeline] Movement &&A not received within"
                   << kMoveStartTimeoutMs << "ms; stopping playback.";
        stop();
    });

    m_motionlessDisplay.setSingleShot(true);
    connect(&m_motionlessDisplay, &QTimer::timeout, this, &TimelineController::onMotionlessDisplayTimeout);
}

TimelineController::~TimelineController() = default;

void TimelineController::setSessionController(SessionController* session) {
    m_session = session;
}

void TimelineController::setRobotController(RobotController* robot) {
    m_robot = robot;
    if (m_robot) {
        connect(m_robot, &RobotController::softwareAckReceived, this, &TimelineController::onRobotSoftwareAck);
        connect(m_robot, &RobotController::finalAckReceived, this, &TimelineController::onRobotFinalAck);
    }
}

void TimelineController::setCommandsController(CommandsController* commands) {
    m_commands = commands;
}

bool TimelineController::isPlaying() const {
    return m_player && m_player->isPlaying();
}

int TimelineController::currentIndex() const {
    return m_player ? m_player->currentIndex() : -1;
}

int TimelineController::currentChipIndex() const {
    return m_player ? m_player->currentChipIndex() : -1;
}

bool TimelineController::commandSupportsDirection(const QString& name) const {
    return supportsFrontBackDirection(name) || supportsLeftRightDirection(name);
}

bool TimelineController::commandUsesFrontBackDirection(const QString& name) const {
    return supportsFrontBackDirection(name);
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
            case zowi::TimelineDirection::Back:
                map["direction"] = "Back";
                break;
            default:
                map["direction"] = "Front";
                break;
        }

        // Compute supportsDuration / supportsDirection (mirrors GameTimelineScreen logic)
        if (cmd.type == zowi::TimelineItemType::Movement) {
            map["supportsDuration"] = true;
            map["supportsDirection"] = commandSupportsDirection(QString::fromStdString(cmd.name));
        } else if (cmd.type == zowi::TimelineItemType::Mouth) {
            map["supportsDuration"] = true;
            map["supportsDirection"] = false;
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
        if (name == "Bend Forward") {
            QString dir = cmd.value("direction", "Front").toString();
            return dir == "Back" ? m_commands->bendBackward(duration)
                                 : m_commands->bendForward(duration);
        }
        if (name == "Shake Leg") {
            QString dir = cmd.value("direction", "Left").toString();
            return dir == "Right" ? m_commands->shakeLegRight(duration)
                                  : m_commands->shakeLegLeft(duration);
        }
        if (name == "Up/Down") return m_commands->updown(duration);
        if (name == "Jitter") return m_commands->jitter(duration);
        if (name == "Swing") return m_commands->swing(duration);
        if (name == "Flapping") {
            QString dir = cmd.value("direction", "Left").toString();
            return dir == "Right" ? m_commands->flappingRight(duration)
                                  : m_commands->flappingLeft(duration);
        }
        if (name == "Crusaito") {
            QString dir = cmd.value("direction", "Front").toString();
            return dir == "Back" ? m_commands->crusaitoBackward(duration)
                                 : m_commands->crusaitoForward(duration);
        }
        if (name == "Jump") return m_commands->jump(duration);
        if (name == "Tiptoе Swing") return m_commands->tiptoeSwing(duration);
    } else if (type == "animation") {
        // Map animation names to gesture IDs (match GestureSelectorContent.qml exactly)
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
        // Map mouth names using CommandsController methods
        if (name == "Smile") return m_commands->mouthById(m_commands->mouthSmile());
        if (name == "HappyOpen") return m_commands->mouthById(m_commands->mouthHappyOpen());
        if (name == "HappyClosed") return m_commands->mouthById(m_commands->mouthHappyClosed());
        if (name == "Heart") return m_commands->mouthById(m_commands->mouthHeart());
        if (name == "BigSurprise") return m_commands->mouthById(m_commands->mouthBigSurprise());
        if (name == "SmallSurprise") return m_commands->mouthById(m_commands->mouthSmallSurprise());
        if (name == "TongueOut") return m_commands->mouthById(m_commands->mouthTongueOut());
        if (name == "Vamp1") return m_commands->mouthById(m_commands->mouthVamp1());
        if (name == "Vamp2") return m_commands->mouthById(m_commands->mouthVamp2());
        if (name == "LineMouth") return m_commands->mouthById(m_commands->mouthLineMouth());
        if (name == "Confused") return m_commands->mouthById(m_commands->mouthConfused());
        if (name == "DiagLeft") return m_commands->mouthById(m_commands->mouthDiagonal());
        if (name == "DiagRight") return m_commands->mouth(17318416);  // reverseDiagonalMatrix value
        if (name == "Sad") return m_commands->mouthById(m_commands->mouthSad());
        if (name == "SadOpen") return m_commands->mouthById(m_commands->mouthSadOpen());
        if (name == "SadClosed") return m_commands->mouthById(m_commands->mouthSadClosed());
        if (name == "Ok") return m_commands->mouthById(m_commands->mouthOk());
        if (name == "X") return m_commands->mouthById(m_commands->mouthX());
        if (name == "Interrogation") return m_commands->mouthById(m_commands->mouthInterrogation());
        if (name == "Thunder") return m_commands->mouthById(m_commands->mouthThunder());
        if (name == "Culito") return m_commands->mouthById(m_commands->mouthCulito());
        if (name == "Angry") return m_commands->mouthById(m_commands->mouthAngry());
    }
    return QString();
}

void TimelineController::play(const QVariantList &items) {
    if (!m_robot || !m_robot->isConnected()) {
        qWarning() << "[Timeline] Cannot play: robot not connected";
        return;
    }

    // Build TimelineStep vector from QVariantList, expanding repetitions
    // (each "repetition" becomes a separate command, like Android does)
    std::vector<zowi::TimelineStep> steps;
    int chipIndex = 0;
    for (const auto& item : items) {
        auto map = item.toMap();
        QString cmdStr = commandToString(map);
        if (cmdStr.isEmpty()) {
            qWarning() << "[Timeline] Skipping unknown command:" << map.value("type") << map.value("name");
            ++chipIndex;   // maintains alignment with timelineModel even for unresolved commands
            continue;
        }

        bool isMovement = (map.value("type", "").toString() == "movement");
        int reps = map.value("reps", 1).toInt();
        zowi::MovementSpeed speed = static_cast<zowi::MovementSpeed>(getDurationMs(map.value("duration", "Medium").toString()));

        qDebug() << "[Timeline] Step:" << map.value("type") << map.value("name")
                  << "reps=" << reps << "cmd=" << cmdStr.trimmed();

        // Expand: each repetition is a separate command, tagged with its origin chip index
        for (int rep = 0; rep < reps; ++rep) {
            steps.push_back({cmdStr.toStdString(), isMovement, speed, chipIndex});
        }
        ++chipIndex;
    }

    if (steps.empty()) {
        qWarning() << "[Timeline] Cannot play: no valid commands";
        return;
    }

    // Reset and start player
    m_player->start(steps);
    updateFromPlayer();
    sendNextCommand();
}

void TimelineController::stop() {
    if (!m_player) return;
    m_moveStartTimeout.stop();
    m_motionlessDisplay.stop();
    m_player->cancel();
    if (m_robot) {
        m_robot->sendData(QString::fromStdString(zowi::commandStop()));
    }
    updateFromPlayer();
}

void TimelineController::onRobotSoftwareAck() {
    if (!m_player || !m_player->isPlaying()) return;
    qDebug() << "[Timeline] &&A received; index=" << m_player->currentIndex()
              << "isMovement=" << m_player->currentStepIsMovement();
    m_moveStartTimeout.stop();
    m_player->onSoftwareAck();
    sendNextCommand();
}

void TimelineController::onRobotFinalAck() {
    if (!m_player || !m_player->isPlaying()) return;

    bool wasMovement = m_player->currentStepIsMovement();
    qDebug() << "[Timeline] &&F received; index=" << m_player->currentIndex()
              << "isMovement=" << wasMovement;
    m_player->onFinalAck();
    updateFromPlayer();

    if (m_player->isPlaying()) {
        // After movement completes and advances to next step, always send the next command first
        if (wasMovement) {
            // Just completed a movement; send whatever comes next (movement or non-movement)
            sendNextCommand();
        } else {
            // Non-movement just completed: firmware sent &&F confirming execution
            // Start the display duration timer before advancing
            int displayMs = m_player->currentSpeed();
            m_motionlessDisplay.setInterval(displayMs);
            m_motionlessDisplay.start();
        }
    }
}

void TimelineController::onMotionlessDisplayTimeout() {
    if (m_player && m_player->isPlaying()) {
        // Non-movement display time is over; advance to next step and send its command
        m_player->advanceNonmovement();
        sendNextCommand();
        // updateFromPlayer() is called inside sendNextCommand() when the next item is sent
    }
}

void TimelineController::sendNextCommand() {
    if (!m_player || !m_robot) return;

    std::string cmd = m_player->nextRobotCommand();
    if (!cmd.empty()) {
        QString qcmd = QString::fromStdString(cmd);
        m_robot->sendData(qcmd);
        qDebug() << "[Timeline] Sending:" << qcmd.trimmed();

        // Update UI to reflect the currently executing item
        updateFromPlayer();

        // Arm guard timeout only for movements (waiting for hardware &&A)
        if (!qcmd.isEmpty() && qcmd[0] == QLatin1Char('M')) {
            m_moveStartTimeout.start();
        }
        // For non-movements: display timer will be started when &&F ack arrives from firmware
    } else if (m_player->finished()) {
        // Sequence complete: send final stop command
        m_robot->sendData(QString::fromStdString(zowi::commandStop()));
        qDebug() << "[Timeline] Sequence complete, sending final stop";
        m_player->reset();
        updateFromPlayer();
    }
}

void TimelineController::updateFromPlayer() {
    emit isPlayingChanged();
    emit currentIndexChanged();
    emit currentChipIndexChanged();
}
