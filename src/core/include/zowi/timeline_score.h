#pragma once

#include <vector>

#include <zowi/timeline_command.h>

namespace zowi {

// Weighted-complexity score for a completed Timeline sequence (the Android
// original has no Timeline ranking). Pure and Qt-free; award it only after
// the whole sequence ran to completion.
struct TimelineScoreConfig {
    int movementPoints = 3;     // per repetition
    int animationPoints = 2;    // per repetition
    int mouthPoints = 1;        // once (mouths ignore repetitions)
    int fastBonusPercent = 20;  // movement speed multiplier: Fast +20%
    int slowPenaltyPercent = 20; // Slow -20%
    int varietyBonus = 2;       // per distinct item type used (max 3 types)
    int maxSteps = 40;          // only the first N steps are scored
    int minSteps = 5;           // shortest sequence allowed in the ranking
};

int timelineScore(const std::vector<TimelineCommand> &commands,
                  const TimelineScoreConfig &config = {});

// True when the sequence is long enough to enter the ranking.
bool timelineQualifiesForRanking(const std::vector<TimelineCommand> &commands,
                                 const TimelineScoreConfig &config = {});

// Highest score the formula can produce (reps are capped at 5 in the UI);
// useful as a plausibility cap when validating online submissions.
int timelineMaxPlausibleScore(const TimelineScoreConfig &config = {});

} // namespace zowi
