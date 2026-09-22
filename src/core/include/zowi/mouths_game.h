#pragma once

#include <cstdint>
#include <vector>

#include <zowi/robot_commands.h>

namespace zowi {

enum class MouthsGameState {
    Idle = 0,        // no game running (fresh controller / after reset)
    RoundActive = 1, // a target mouth is shown, player may draw
    RoundSolved = 2, // draw matched; victory intermission before next round
    GameOver = 3     // countdown expired
};

// Difficulty bands: mouths the game can pick as targets. Band `i` unlocks at
// `bandUnlockLevels[i]`; the target for a round is drawn ONLY from the band
// that unlocks at the current level (bands are ordered from easiest to draw —
// few LEDs / symmetric — to hardest). Reaching the next unlock level moves the
// game onto the harder band, so difficulty never regresses. Tuned for the
// desktop: the Android original (23 patterns across 4 types, random within
// type) is simplified to a progressive pool of 20 mouths.
struct MouthsGameConfig {
    // Very easy (levels 1+): 4–10 LEDs, mostly symmetric.
    std::vector<MouthId> band0 = {
        MouthId::Smile, MouthId::Ok, MouthId::SmallSurprise, MouthId::HappyClosed
    };
    // Easy (levels 3+): simple lines/arcs.
    std::vector<MouthId> band1 = {
        MouthId::LineMouth, MouthId::HappyOpen, MouthId::Sad, MouthId::Confused
    };
    // Medium (levels 5+): open mouths, asymmetric saddles.
    std::vector<MouthId> band2 = {
        MouthId::SadOpen, MouthId::SadClosed, MouthId::Angry, MouthId::Thunder
    };
    // Hard (levels 8+): heart, open/closed mimics, tongue, interrogation.
    std::vector<MouthId> band3 = {
        MouthId::Heart, MouthId::BigSurprise, MouthId::TongueOut, MouthId::Interrogation
    };
    // Very hard (levels 11+): dense / scattered patterns.
    std::vector<MouthId> band4 = {
        MouthId::X, MouthId::Vamp1, MouthId::Culito, MouthId::Diagonal
    };
    std::vector<int> bandUnlockLevels = { 1, 3, 5, 8, 11 };

    // Countdown: 10 s on levels 1–3, then −1 s every 4th level, floored at 2 s.
    int initialCountdownMs = 10000;
    int countdownStepMs = 1000;
    int countdownStepEveryLevels = 4;
    int minCountdownMs = 2000;

    // Level at which a solved round unlocks the mouths_editor achievement
    // (deferred ACHIEVEMENTS layer; Android threshold uses score, but the
    // desktop key is level): "acertar ocho bocas" = level 8.
    int achievementLevelThreshold = 8;
    // Score (level − 1) at which a round enters the game's ranking layer
    // (deferred shared RankingController; Android MIN_SCORE_TO_RANK).
    int rankScoreThreshold = 2;
};

class MouthsGame {
public:
    explicit MouthsGame(const MouthsGameConfig& config = {});
    ~MouthsGame() = default;

    void startGame();   // fresh game: level 1, RoundActive, random target
    // (never the previous *game's* first target)
    void reset();       // → Idle, level 0
    // Live draw comparison. `matrix` is the 32-bit mouth pattern built from
    // the 6×5 grid (bit 29−i for cell i); only the lower 30 bits are used.
    // True when it matches the target → RoundSolved. False while Idle,
    // GameOver, already solved, or on mismatch.
    bool submitDraw(unsigned long matrix);
    // RoundSolved → next round: level+1, new random target (never equal to the
    // one just solved — no two consecutive rounds show the same mouth),
    // RoundActive.
    void advanceLevel();
    void onTimeout();   // RoundActive → GameOver; score = level − 1

    MouthsGameState state() const { return m_state; }
    int level() const { return m_level; }
    int score() const { return m_score; }
    unsigned long targetPattern() const { return m_targetPattern; }
    // Mouth set for the current target (for the miniature).
    MouthId target() const { return m_target; }
    int countdownMsForLevel(int level) const;
    // Reserved (deferred layers): achievement/ranking gates as pure predicates.
    bool shouldUnlockAchievement() const { return m_level >= m_config.achievementLevelThreshold; }
    bool qualifiesForRanking() const { return m_score >= m_config.rankScoreThreshold; }

private:
    // Picks the round target from the band unlocking at the current level,
    // avoiding every mouth in `forbidden`. When the whole band is forbidden
    // (single-mouth bands make exclusion impossible) it falls back to any
    // mouth of the band.
    void pickTarget(const std::vector<MouthId>& forbidden);
    int unlockedBandCount(int level) const;

    MouthsGameConfig m_config;
    MouthsGameState m_state = MouthsGameState::Idle;
    int m_level = 0;
    int m_score = 0;
    MouthId m_target = MouthId::Smile;
    unsigned long m_targetPattern = 0;
    // First target of the previous game (so two consecutive games never start
    // with the same mouth) — persisted for the lifetime of this instance.
    MouthId m_previousFirstTarget = MouthId::Smile;
    bool m_hasPreviousGame = false;
    unsigned long m_seedState = 0x9e3779b9UL;
};

} // namespace zowi
