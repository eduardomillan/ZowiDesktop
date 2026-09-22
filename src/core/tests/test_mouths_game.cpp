#include "zowi/mouths_game.h"
#include "zowi/robot_commands.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace zowi;

static int failures = 0;

static void check(bool actual, bool expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << (actual ? "true" : "false")
                  << " expected " << (expected ? "true" : "false") << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkEqual(int actual, int expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << actual << " expected " << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkUL(unsigned long actual, unsigned long expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << actual << " expected " << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

// Deterministic config: one mouth per band, one band unlocked per level.
// The target is drawn only from the band that unlocks at the current level,
// so every round is fully predictable.
static MouthsGameConfig testConfig() {
    MouthsGameConfig cfg;
    cfg.band0 = { MouthId::Smile };
    cfg.band1 = { MouthId::HappyOpen };
    cfg.band2 = { MouthId::LineMouth };
    cfg.band3 = {};
    cfg.band4 = {};
    cfg.bandUnlockLevels = { 1, 2, 3 };
    return cfg;
}

int main() {
    // Basic flow
    {
        MouthsGame g(testConfig());
        check(g.state() == MouthsGameState::Idle, true, "fresh game is Idle");
        g.startGame();
        check(g.state() == MouthsGameState::RoundActive, true, "startGame -> RoundActive");
        checkEqual(g.level(), 1, "level 1");
        checkEqual(g.score(), 0, "score 0 at level 1");
        check(g.target() == MouthId::Smile, true, "band 0 target at level 1");
        checkUL(g.targetPattern(), mouthPatternForId(MouthId::Smile), "target pattern matches");
        checkEqual(g.countdownMsForLevel(1), 10000, "10 s countdown on level 1");
    }

    // Wrong draw keeps the round active
    {
        MouthsGame g(testConfig());
        g.startGame();
        check(g.submitDraw(0), false, "all-zero draw does not match");
        check(g.state() == MouthsGameState::RoundActive, true, "still active after wrong draw");
    }

    // Correct draw -> RoundSolved; a second draw is ignored
    {
        MouthsGame g(testConfig());
        g.startGame();
        check(g.submitDraw(mouthPatternForId(MouthId::Smile)), true, "correct draw solves the round");
        check(g.state() == MouthsGameState::RoundSolved, true, "RoundSolved state");
        check(g.submitDraw(mouthPatternForId(MouthId::Smile)), false, "second draw after solve ignored");
    }

    // Levels advance only from RoundSolved; one new band per level
    {
        MouthsGame g(testConfig());
        g.startGame();
        g.advanceLevel(); // no-op: round not solved yet
        checkEqual(g.level(), 1, "advance without solve keeps level");
        g.submitDraw(mouthPatternForId(MouthId::Smile));
        g.advanceLevel();
        checkEqual(g.level(), 2, "advance -> level 2");
        check(g.state() == MouthsGameState::RoundActive, true, "new round active");
        check(g.target() == MouthId::HappyOpen, true, "band 1 target at level 2");
        g.submitDraw(mouthPatternForId(MouthId::HappyOpen));
        g.advanceLevel();
        check(g.target() == MouthId::LineMouth, true, "band 2 target at level 3");
        g.onTimeout();
        checkEqual(g.score(), 2, "score = level - 1 at game over");
        check(g.qualifiesForRanking(), true, "level 3 (score 2) qualifies for ranking");
    }

    // Countdown formula: -1 s every 4th level, floored at 2 s
    {
        MouthsGame g(testConfig());
        checkEqual(g.countdownMsForLevel(1), 10000, "L1 -> 10 s");
        checkEqual(g.countdownMsForLevel(3), 10000, "L3 -> 10 s");
        checkEqual(g.countdownMsForLevel(4), 9000, "L4 -> 9 s");
        checkEqual(g.countdownMsForLevel(8), 8000, "L8 -> 8 s");
        checkEqual(g.countdownMsForLevel(33), 2000, "L33 floored at 2 s");
    }

    // Timeout -> GameOver, score = level - 1, reserved thresholds
    {
        MouthsGame g(testConfig());
        g.startGame();
        for (int i = 0; i < 7; ++i) {
            g.submitDraw(g.targetPattern());
            g.advanceLevel();
        }
        checkEqual(g.level(), 8, "level 8 reached after 7 solved rounds");
        check(g.shouldUnlockAchievement(), true, "level >= 8 flagged at level 8");
        g.onTimeout();
        check(g.state() == MouthsGameState::GameOver, true, "timeout -> GameOver");
        checkEqual(g.score(), 7, "score = level - 1");
        check(g.shouldUnlockAchievement(), true, "level >= 8 at game over unlocks mouths_editor");
        g.onTimeout();
        check(g.state() == MouthsGameState::GameOver, true, "double timeout ignored");
    }

    // Low-level game over: no achievement, no ranking eligibility
    {
        MouthsGame g(testConfig());
        g.startGame();
        g.onTimeout();
        checkEqual(g.score(), 0, "timeout at level 1 -> score 0");
        check(g.shouldUnlockAchievement(), false, "level 1 unlocks nothing");
        check(g.qualifiesForRanking(), false, "score 0 does not qualify for ranking");
    }

    // Reset
    {
        MouthsGame g(testConfig());
        g.startGame();
        g.onTimeout();
        g.reset();
        check(g.state() == MouthsGameState::Idle, true, "reset -> Idle");
        checkEqual(g.level(), 0, "reset level 0");
    }

    if (failures == 0) {
        std::cout << "\nAll mouths_game tests passed.\n";
        return 0;
    }
    std::cerr << "\n" << failures << " mouths_game test(s) failed.\n";
    return 1;
}