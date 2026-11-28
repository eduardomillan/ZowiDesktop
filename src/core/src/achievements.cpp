#include <zowi/achievements.h>

#include <algorithm>

#include <nlohmann/json.hpp>

#include <zowi/session_store.h>

namespace zowi {

namespace {

using json = nlohmann::json;

constexpr const char *kUnlockedKey = "unlocked";   // JSON array of ids
constexpr const char *kStartsKey = "app_starts";
constexpr const char *kDaysKey = "use_days";       // JSON array of "YYYY-MM-DD"
constexpr const char *kVisitedKey = "visited";     // JSON array of game ids

SessionStore openStore(const std::string &configDir)
{
    return SessionStore("ZowiDesktop", "ZowiAchievements", configDir);
}

std::vector<std::string> readStrings(const SessionStore &store, const char *key)
{
    std::vector<std::string> out;
    const json parsed = json::parse(store.getString(key, "[]"), nullptr, false);
    if (!parsed.is_array()) return out;
    for (const auto &item : parsed) {
        if (item.is_string()) out.push_back(item.get<std::string>());
    }
    return out;
}

void writeStrings(SessionStore &store, const char *key, const std::vector<std::string> &values)
{
    json arr = json::array();
    for (const auto &v : values) arr.push_back(v);
    store.setString(key, arr.dump());
}

bool contains(const std::vector<std::string> &values, const std::string &value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

} // namespace

const char *achievementTypeName(AchievementType type)
{
    switch (type) {
    case AchievementType::Movement:  return "movement";
    case AchievementType::Animation: return "animation";
    case AchievementType::Game:      return "game";
    }
    return "movement";
}

// ── rules ───────────────────────────────────────────────────────────────────

std::vector<std::string> achievementsForAppStart(bool notFirstUsage, int daysOfUse,
                                                 const AchievementRules &rules)
{
    std::vector<std::string> ids;
    if (notFirstUsage) ids.push_back("crusaito");
    if (daysOfUse >= rules.daysOfUse) ids.push_back("fart");
    return ids;
}

bool shakeLegOnFirstVisit(bool firstVisitOfThis, bool otherAlreadyVisited)
{
    return firstVisitOfThis && otherAlreadyVisited;
}

bool unlocksAnxious(int chipsPlayed, const AchievementRules &rules)
{
    return chipsPlayed >= rules.timelineChips;
}

bool unlocksInLove(int memoryScore, const AchievementRules &rules)
{
    return memoryScore >= rules.memoryScore;
}

bool unlocksMouthsEditor(int mouthsScore, const AchievementRules &rules)
{
    // score = level - 1, so reaching level N means a score of N - 1.
    return mouthsScore + 1 >= rules.mouthsLevel;
}

// ── store ───────────────────────────────────────────────────────────────────

const std::vector<Achievement> &AchievementsStore::catalog()
{
    static const std::vector<Achievement> kCatalog = {
        {"crusaito", AchievementType::Movement, false},
        {"flapping", AchievementType::Movement, false},
        {"shake_leg", AchievementType::Movement, false},
        {"tip_toe", AchievementType::Movement, false},
        {"jitter", AchievementType::Movement, false},
        {"ascending_turn", AchievementType::Movement, true},
        {"swing", AchievementType::Movement, false},
        {"super_happy", AchievementType::Animation, false},
        {"sleepy", AchievementType::Animation, false},
        {"fart", AchievementType::Animation, false},
        {"confused", AchievementType::Animation, false},
        {"in_love", AchievementType::Animation, false},
        {"angry", AchievementType::Animation, false},
        {"anxious", AchievementType::Animation, false},
        {"magic", AchievementType::Animation, false},
        {"wave", AchievementType::Animation, false},
        {"mouths_editor", AchievementType::Game, false},
    };
    return kCatalog;
}

bool AchievementsStore::isKnown(const std::string &id)
{
    const auto &c = catalog();
    return std::any_of(c.begin(), c.end(), [&](const Achievement &a) { return a.id == id; });
}

AchievementsStore::AchievementsStore(const std::string &configDir) : m_configDir(configDir) {}

std::vector<Achievement> AchievementsStore::list() const
{
    SessionStore store = openStore(m_configDir);
    // Never saved: the initial state. Otherwise the saved ids (unknown ones are ignored).
    const bool saved = !store.getRaw(kUnlockedKey).empty();
    const auto unlocked = readStrings(store, kUnlockedKey);

    std::vector<Achievement> out = catalog();
    if (saved) {
        for (auto &a : out) a.unlocked = contains(unlocked, a.id);
    }
    return out;
}

bool AchievementsStore::isUnlocked(const std::string &id) const
{
    for (const auto &a : list()) {
        if (a.id == id) return a.unlocked;
    }
    return false;
}

int AchievementsStore::unlockedCount() const
{
    const auto all = list();
    return static_cast<int>(std::count_if(all.begin(), all.end(),
                                          [](const Achievement &a) { return a.unlocked; }));
}

bool AchievementsStore::unlock(const std::string &id)
{
    if (!isKnown(id)) return false;
    auto all = list();
    std::vector<std::string> ids;
    bool changed = false;
    for (auto &a : all) {
        if (a.id == id && !a.unlocked) {
            a.unlocked = true;
            changed = true;
        }
        if (a.unlocked) ids.push_back(a.id);
    }
    if (!changed) return false;
    SessionStore store = openStore(m_configDir);
    writeStrings(store, kUnlockedKey, ids);
    return true;
}

int AchievementsStore::unlockAll()
{
    const auto all = list();
    int newly = 0;
    std::vector<std::string> ids;
    for (const auto &a : all) {
        if (!a.unlocked) ++newly;
        ids.push_back(a.id);
    }
    if (newly > 0) {
        SessionStore store = openStore(m_configDir);
        writeStrings(store, kUnlockedKey, ids);
    }
    return newly;
}

void AchievementsStore::reset()
{
    SessionStore store = openStore(m_configDir);
    std::vector<std::string> ids;
    for (const auto &a : catalog()) {
        if (a.unlocked) ids.push_back(a.id);   // the initially unlocked ones
    }
    writeStrings(store, kUnlockedKey, ids);
    store.setInt(kStartsKey, 0);
    writeStrings(store, kDaysKey, {});
    writeStrings(store, kVisitedKey, {});
}

AchievementsStore::AppStart AchievementsStore::logAppStart(const std::string &isoDate)
{
    SessionStore store = openStore(m_configDir);
    AppStart result;
    const int starts = store.getInt(kStartsKey, 0);
    result.notFirstUsage = starts > 0;
    store.setInt(kStartsKey, starts + 1);

    auto days = readStrings(store, kDaysKey);
    if (!isoDate.empty() && !contains(days, isoDate)) {
        days.push_back(isoDate);
        writeStrings(store, kDaysKey, days);
    }
    result.daysOfUse = static_cast<int>(days.size());
    return result;
}

bool AchievementsStore::hasVisited(const std::string &game) const
{
    SessionStore store = openStore(m_configDir);
    return contains(readStrings(store, kVisitedKey), game);
}

bool AchievementsStore::markVisited(const std::string &game)
{
    SessionStore store = openStore(m_configDir);
    auto visited = readStrings(store, kVisitedKey);
    if (contains(visited, game)) return false;
    visited.push_back(game);
    writeStrings(store, kVisitedKey, visited);
    return true;
}

} // namespace zowi
