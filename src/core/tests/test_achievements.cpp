#include <zowi/achievements.h>

#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>

namespace fs = std::filesystem;
using zowi::AchievementsStore;

namespace {

struct TempDir {
    fs::path dir;
    explicit TempDir(const char *name)
        : dir(fs::temp_directory_path() / (std::string("zowi_achievements_test_") + name)) {
        fs::remove_all(dir);
        fs::create_directories(dir);
    }
    ~TempDir() { fs::remove_all(dir); }
};

} // namespace

void test_catalog()
{
    std::cout << "test_catalog: " << std::flush;
    const auto &c = AchievementsStore::catalog();
    assert(c.size() == 17);
    assert(AchievementsStore::total() == 17);
    std::set<std::string> ids;
    int movements = 0, animations = 0, games = 0, unlocked = 0;
    for (const auto &a : c) {
        ids.insert(a.id);
        if (a.type == zowi::AchievementType::Movement) ++movements;
        if (a.type == zowi::AchievementType::Animation) ++animations;
        if (a.type == zowi::AchievementType::Game) ++games;
        if (a.unlocked) ++unlocked;
    }
    assert(ids.size() == 17);                // no duplicates
    assert(movements == 7 && animations == 9 && games == 1);
    assert(unlocked == 1);                   // only ascending_turn at the start
    assert(AchievementsStore::isKnown("crusaito"));
    assert(!AchievementsStore::isKnown("nope"));
    assert(std::string(zowi::achievementTypeName(zowi::AchievementType::Game)) == "game");
    std::cout << "ok\n";
}

void test_initial_state_and_unlock()
{
    std::cout << "test_initial_state_and_unlock: " << std::flush;
    TempDir tmp("unlock");
    AchievementsStore s(tmp.dir.string());
    assert(s.unlockedCount() == 1);
    assert(s.isUnlocked("ascending_turn"));
    assert(!s.isUnlocked("crusaito"));

    assert(s.unlock("crusaito"));            // newly unlocked
    assert(!s.unlock("crusaito"));           // already unlocked
    assert(!s.unlock("nope"));               // unknown id
    assert(s.isUnlocked("crusaito"));
    assert(s.unlockedCount() == 2);

    AchievementsStore again(tmp.dir.string());   // persisted in its own file
    assert(again.isUnlocked("crusaito") && again.isUnlocked("ascending_turn"));
    assert(again.unlockedCount() == 2);
    std::cout << "ok\n";
}

void test_unlock_all_and_reset()
{
    std::cout << "test_unlock_all_and_reset: " << std::flush;
    TempDir tmp("all");
    AchievementsStore s(tmp.dir.string());
    s.unlock("wave");
    assert(s.unlockAll() == 17 - 2);         // ascending_turn + wave were already open
    assert(s.unlockedCount() == 17);
    assert(s.unlockAll() == 0);

    s.logAppStart("2026-11-01");
    s.markVisited(zowi::kGamepadGame);
    s.reset();
    assert(s.unlockedCount() == 1);
    assert(s.isUnlocked("ascending_turn"));
    assert(!s.hasVisited(zowi::kGamepadGame));
    assert(!s.logAppStart("2026-11-02").notFirstUsage);   // counters were wiped too
    std::cout << "ok\n";
}

void test_usage_tracking()
{
    std::cout << "test_usage_tracking: " << std::flush;
    TempDir tmp("usage");
    AchievementsStore s(tmp.dir.string());

    auto first = s.logAppStart("2026-11-01");
    assert(!first.notFirstUsage && first.daysOfUse == 1);
    auto sameDay = s.logAppStart("2026-11-01");
    assert(sameDay.notFirstUsage && sameDay.daysOfUse == 1);     // same day: not a new day
    auto next = s.logAppStart("2026-11-02");
    assert(next.notFirstUsage && next.daysOfUse == 2);

    assert(!s.hasVisited(zowi::kTimelineGame));
    assert(s.markVisited(zowi::kTimelineGame));                  // first visit
    assert(!s.markVisited(zowi::kTimelineGame));                 // already
    assert(s.hasVisited(zowi::kTimelineGame));
    assert(!s.hasVisited(zowi::kGamepadGame));
    std::cout << "ok\n";
}

void test_rules()
{
    std::cout << "test_rules: " << std::flush;
    using namespace zowi;
    AchievementRules rules;   // 4 days, 15 chips, 12 points, level 8

    assert(achievementsForAppStart(false, 1, rules).empty());                // first ever launch
    auto second = achievementsForAppStart(true, 2, rules);
    assert(second.size() == 1 && second[0] == "crusaito");
    auto fourth = achievementsForAppStart(true, 4, rules);
    assert(fourth.size() == 2 && fourth[1] == "fart");
    AchievementRules relaxed; relaxed.daysOfUse = 2;
    assert(achievementsForAppStart(false, 2, relaxed).size() == 1);          // configurable

    assert(!shakeLegOnFirstVisit(true, false));    // first feature tried
    assert(shakeLegOnFirstVisit(true, true));      // second distinct feature
    assert(!shakeLegOnFirstVisit(false, true));    // not a first visit

    assert(!unlocksAnxious(14, rules) && unlocksAnxious(15, rules));
    assert(!unlocksInLove(11, rules) && unlocksInLove(12, rules));
    assert(!unlocksMouthsEditor(6, rules));        // level 7
    assert(unlocksMouthsEditor(7, rules));         // level 8
    std::cout << "ok\n";
}

void test_fresh_directory_starts_initial()
{
    std::cout << "test_fresh_directory_starts_initial: " << std::flush;
    TempDir tmp("corrupt");
    {
        std::filesystem::create_directories(tmp.dir);
        // A directory with no saved file: the initial state.
        zowi::AchievementsStore s(tmp.dir.string());
        assert(s.unlockedCount() == 1);
    }
    std::cout << "ok\n";
}

int main()
{
    test_catalog();
    test_initial_state_and_unlock();
    test_unlock_all_and_reset();
    test_usage_tracking();
    test_rules();
    test_fresh_directory_starts_initial();
    std::cout << "All achievements tests passed.\n";
    return 0;
}
