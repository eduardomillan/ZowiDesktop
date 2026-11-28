#pragma once

#include <string>
#include <vector>

namespace zowi {

// Achievements layer (ported from ZowiAppReborn): 17 collectible achievements
// of three kinds. Unlocking one can open content (the gamepad movements, the
// Mouths Editor) and shows a notice. Qt-free; the GUI controller wraps it.
//
// Persistence is its own file (ZowiAchievements.json, next to ZowiApp.json), so
// `session clear` never touches the progress, like the ranking.

enum class AchievementType { Movement, Animation, Game };

struct Achievement {
    std::string id;       // "crusaito", "super_happy", "mouths_editor", ...
    AchievementType type = AchievementType::Movement;
    bool unlocked = false;
};

// Thresholds of the unlock rules (the config keys live in config.json).
struct AchievementRules {
    int daysOfUse = 4;       // "fart": distinct days the app was opened
    int timelineChips = 15;  // "anxious": chips of a Timeline sequence played to the end
    int memoryScore = 12;    // "in_love": Zowi says score (sequence length - 1)
    int mouthsLevel = 8;     // "mouths_editor": level reached in Draw the mouths
};

// Games whose first visit is tracked (the "shake_leg" rule needs both).
inline constexpr const char *kGamepadGame = "gamepad";
inline constexpr const char *kTimelineGame = "timeline";

const char *achievementTypeName(AchievementType type);  // "movement" | "animation" | "game"

// ── Pure unlock rules (no state, unit-tested) ───────────────────────────────
// App start: `crusaito` from the second launch on, `fart` after enough distinct days.
std::vector<std::string> achievementsForAppStart(bool notFirstUsage, int daysOfUse,
                                                 const AchievementRules &rules = {});
// `shake_leg` unlocks when the first visit of the gamepad happens after the
// Timeline was already visited, or the other way round (i.e. on the second
// distinct feature the player tries).
bool shakeLegOnFirstVisit(bool firstVisitOfThis, bool otherAlreadyVisited);
bool unlocksAnxious(int chipsPlayed, const AchievementRules &rules = {});
bool unlocksInLove(int memoryScore, const AchievementRules &rules = {});
// `score` is the Draw the mouths score (level - 1).
bool unlocksMouthsEditor(int mouthsScore, const AchievementRules &rules = {});

class AchievementsStore {
public:
    explicit AchievementsStore(const std::string &configDir = "");

    // The 17 achievements in canonical order with their initial state
    // (only `ascending_turn` starts unlocked, as in the Android app).
    static const std::vector<Achievement> &catalog();
    static bool isKnown(const std::string &id);
    static int total() { return static_cast<int>(catalog().size()); }

    std::vector<Achievement> list() const;          // catalog order, current state
    bool isUnlocked(const std::string &id) const;
    int unlockedCount() const;

    // True only when the achievement was locked and is now unlocked; false for
    // an unknown id or one already unlocked.
    bool unlock(const std::string &id);
    int unlockAll();                                 // how many were newly unlocked
    // Back to the initial state: locks everything but the initial ones and
    // forgets the usage counters and first visits. Rankings are untouched.
    void reset();

    // ── usage tracking (inputs of the rules) ────────────────────────────
    struct AppStart {
        bool notFirstUsage = false;  // the app had been opened before
        int daysOfUse = 0;           // distinct days, including today
    };
    // Records an app start on `isoDate` ("YYYY-MM-DD") and returns the counters.
    AppStart logAppStart(const std::string &isoDate);
    bool hasVisited(const std::string &game) const;
    // Marks the first visit of `game`; true when it was the first one.
    bool markVisited(const std::string &game);

private:
    std::string m_configDir;
};

} // namespace zowi
