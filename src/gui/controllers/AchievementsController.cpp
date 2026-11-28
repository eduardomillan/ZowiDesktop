#include "AchievementsController.h"

#include <QDate>
#include <QDebug>

#include <zowi/mouths_game.h>
#include <zowi/robot_commands.h>
#include <zowi/zowi_dice.h>

#include <zowi/config_store.h>

#include "ConfigController.h"
#include "RobotController.h"

AchievementsController::Settings AchievementsController::settingsFrom(const ConfigController &config)
{
    auto flag = [&](const char *key, bool fallback) {
        return zowi::ConfigStore::parseBool(config.get(QString::fromLatin1(key)).toStdString(), fallback);
    };
    Settings s;
    s.enabled = flag("achievements_enabled", true);
    s.lockContent = flag("achievements_lock_content", true);
    s.notify = flag("achievements_notify", true);
    s.robotCelebration = flag("achievements_robot_celebration", true);
    s.easterEgg = flag("achievements_easter_egg", true);
    bool ok = false;
    const int days = config.get(QStringLiteral("achievements_days_of_use")).toInt(&ok);
    if (ok && days > 0) s.daysOfUse = days;
    return s;
}

AchievementsController::AchievementsController(const Settings &settings, QObject *parent)
    : QObject(parent)
    , m_settings(settings)
{
    // Keep the rule thresholds in sync with the games' own configuration.
    m_rules.daysOfUse = settings.daysOfUse > 0 ? settings.daysOfUse : 4;
    m_rules.memoryScore = zowi::ZowiDiceConfig{}.achievementThreshold;
    m_rules.mouthsLevel = zowi::MouthsGameConfig{}.achievementLevelThreshold;
    refresh();
}

void AchievementsController::refresh()
{
    QStringList ids;
    for (const auto &a : m_store.list()) {
        if (a.unlocked) ids << QString::fromStdString(a.id);
    }
    if (ids != m_unlocked) {
        m_unlocked = ids;
        emit achievementsChanged();
    }
}

QVariantList AchievementsController::list() const
{
    QVariantList out;
    for (const auto &a : m_store.list()) {
        QVariantMap m;
        m["id"] = QString::fromStdString(a.id);
        m["type"] = QString::fromUtf8(zowi::achievementTypeName(a.type));
        m["unlocked"] = a.unlocked;
        out << m;
    }
    return out;
}

QString AchievementsController::typeOf(const QString &id) const
{
    for (const auto &a : zowi::AchievementsStore::catalog()) {
        if (QString::fromStdString(a.id) == id) return QString::fromUtf8(zowi::achievementTypeName(a.type));
    }
    return QString();
}

bool AchievementsController::isUnlocked(const QString &id) const
{
    return m_unlocked.contains(id);
}

bool AchievementsController::available(const QString &id) const
{
    return !locking() || m_unlocked.contains(id);
}

bool AchievementsController::unlock(const QString &id)
{
    if (!m_settings.enabled) return false;
    if (!m_store.unlock(id.toStdString())) return false;
    qInfo() << "[achievements] unlocked" << id;
    refresh();
    emit achievementUnlocked(id);
    celebrate();
    return true;
}

void AchievementsController::celebrate()
{
    if (!m_settings.robotCelebration || !m_robot || !m_robot->isConnected()) return;
    m_robot->sendData(QString::fromStdString(zowi::commandGesture(zowi::GestureId::Victory)));
}

void AchievementsController::appStarted()
{
    if (!m_settings.enabled) return;
    const auto start = m_store.logAppStart(QDate::currentDate().toString(Qt::ISODate).toStdString());
    for (const auto &id : zowi::achievementsForAppStart(start.notFirstUsage, start.daysOfUse, m_rules))
        unlock(QString::fromStdString(id));
}

void AchievementsController::gamepadOpened()
{
    if (!m_settings.enabled) return;
    const bool timelineSeen = m_store.hasVisited(zowi::kTimelineGame);
    const bool first = m_store.markVisited(zowi::kGamepadGame);
    if (zowi::shakeLegOnFirstVisit(first, timelineSeen)) unlock(QStringLiteral("shake_leg"));
}

void AchievementsController::timelineOpened()
{
    if (!m_settings.enabled) return;
    const bool gamepadSeen = m_store.hasVisited(zowi::kGamepadGame);
    const bool first = m_store.markVisited(zowi::kTimelineGame);
    if (zowi::shakeLegOnFirstVisit(first, gamepadSeen)) unlock(QStringLiteral("shake_leg"));
}

void AchievementsController::projectCompleted(const QString &achievementId)
{
    // Projects carry the id of the achievement they award; an unknown id (for
    // example a new project without one) simply awards nothing.
    if (achievementId.isEmpty()) return;
    unlock(achievementId);
}

void AchievementsController::onMemoryGameOver(int score)
{
    if (zowi::unlocksInLove(score, m_rules)) unlock(QStringLiteral("in_love"));
}

void AchievementsController::onMouthsGameOver(int score)
{
    if (zowi::unlocksMouthsEditor(score, m_rules)) unlock(QStringLiteral("mouths_editor"));
}

void AchievementsController::onTimelineCompleted(int score, bool eligible, int chips)
{
    Q_UNUSED(score)
    Q_UNUSED(eligible)
    if (zowi::unlocksAnxious(chips, m_rules)) unlock(QStringLiteral("anxious"));
}

void AchievementsController::easterEggStep(int step)
{
    if (!easterEggEnabled()) return;
    if (step == 1) {
        m_egg1 = true;
    } else if (step == 2) {
        if (m_egg1) m_egg2 = true;
    } else if (step == 3) {
        if (m_egg1 && m_egg2) {
            m_egg1 = m_egg2 = false;
            const int newly = m_store.unlockAll();
            qInfo() << "[achievements] easter egg: unlocked" << newly;
            refresh();
            celebrate();
        }
    }
}

void AchievementsController::showLockedHint(const QString &id)
{
    if (locking() && !m_unlocked.contains(id)) emit lockedHint(id);
}

void AchievementsController::resetAll()
{
    if (!m_settings.enabled) return;
    m_store.reset();
    m_egg1 = m_egg2 = false;
    qInfo() << "[achievements] reset";
    refresh();
}
