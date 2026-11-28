#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <zowi/achievements.h>

class RobotController;
class ConfigController;

// QML bridge over zowi::AchievementsStore: the achievements layer. 17
// collectible achievements (movements, animations, one game) that are unlocked
// by playing; unlocking one shows a notice, celebrates on the robot and can open
// content (the gamepad movements, the Mouths Editor). Its own file
// (ZowiAchievements.json), separate from the session.
//
// The whole layer is switched by config keys (see CONFIG_HOWTO.md):
// `achievements_enabled` turns it off completely (nothing is recorded, shown or
// locked), `achievements_lock_content` decides whether achievements gate content,
// `achievements_notify`, `achievements_robot_celebration`,
// `achievements_days_of_use` and `achievements_easter_egg` tune the rest.
class AchievementsController : public QObject
{
    Q_OBJECT

public:
    struct Settings {
        bool enabled = true;
        bool lockContent = true;
        bool notify = true;
        bool robotCelebration = true;
        bool easterEgg = true;
        int daysOfUse = 4;
    };

    explicit AchievementsController(const Settings &settings, QObject *parent = nullptr);
    // Reads the `achievements_*` keys of config.json (with its override layers).
    static Settings settingsFrom(const ConfigController &config);

    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    Q_PROPERTY(bool locking READ locking CONSTANT)  // enabled and achievements gate content
    Q_PROPERTY(bool notifyEnabled READ notifyEnabled CONSTANT)
    Q_PROPERTY(bool easterEggEnabled READ easterEggEnabled CONSTANT)
    Q_PROPERTY(QStringList unlockedIds READ unlockedIds NOTIFY achievementsChanged)
    Q_PROPERTY(int unlockedCount READ unlockedCount NOTIFY achievementsChanged)
    Q_PROPERTY(int total READ total CONSTANT)
    // Thresholds of the unlock rules, for the "how to unlock" texts.
    Q_PROPERTY(int daysOfUse READ daysOfUse CONSTANT)
    Q_PROPERTY(int timelineChips READ timelineChips CONSTANT)
    Q_PROPERTY(int memoryScore READ memoryScore CONSTANT)
    Q_PROPERTY(int mouthsLevel READ mouthsLevel CONSTANT)

    bool enabled() const { return m_settings.enabled; }
    bool locking() const { return m_settings.enabled && m_settings.lockContent; }
    bool notifyEnabled() const { return m_settings.notify; }
    bool easterEggEnabled() const { return m_settings.enabled && m_settings.easterEgg; }
    QStringList unlockedIds() const { return m_unlocked; }
    int unlockedCount() const { return m_unlocked.size(); }
    int total() const { return zowi::AchievementsStore::total(); }
    int daysOfUse() const { return m_rules.daysOfUse; }
    int timelineChips() const { return m_rules.timelineChips; }
    int memoryScore() const { return m_rules.memoryScore; }
    int mouthsLevel() const { return m_rules.mouthsLevel; }

    // The robot celebrates every unlock with the VICTORY gesture (when connected).
    void setRobotController(RobotController *robot) { m_robot = robot; }

    // [{ id, type ("movement"|"animation"|"game"), unlocked }] in catalog order.
    Q_INVOKABLE QVariantList list() const;
    Q_INVOKABLE bool isUnlocked(const QString &id) const;
    Q_INVOKABLE QString typeOf(const QString &id) const;  // "movement" | "animation" | "game" | ""
    // True when the content tied to `id` can be used: the layer or its locking is
    // off, or the achievement is unlocked.
    Q_INVOKABLE bool available(const QString &id) const;

    // ── events from the app (all no-ops while the layer is disabled) ────
    Q_INVOKABLE void appStarted();                 // once per launch, after the Home screen is up
    Q_INVOKABLE void gamepadOpened();
    Q_INVOKABLE void timelineOpened();
    Q_INVOKABLE void projectCompleted(const QString &achievementId);

    // The hidden corner buttons of the achievements screen: pressing 1, 2 and 3
    // in order unlocks everything.
    Q_INVOKABLE void easterEggStep(int step);
    // A locked control was pressed: the UI explains how to unlock it.
    Q_INVOKABLE void showLockedHint(const QString &id);

    // "Delete achievements": back to the initial state. Rankings are untouched.
    Q_INVOKABLE void resetAll();

public slots:
    void onMemoryGameOver(int score);
    void onMouthsGameOver(int score);
    void onTimelineCompleted(int score, bool eligible, int chips);

signals:
    void achievementsChanged();
    void achievementUnlocked(const QString &id);
    void lockedHint(const QString &id);

private:
    bool unlock(const QString &id);
    void refresh();
    void celebrate();

    Settings m_settings;
    zowi::AchievementRules m_rules;
    zowi::AchievementsStore m_store;
    QStringList m_unlocked;
    RobotController *m_robot = nullptr;
    bool m_egg1 = false;
    bool m_egg2 = false;
};
