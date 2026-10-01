#include "zowi/timeline_score.h"

#include <algorithm>
#include <set>

namespace zowi {

namespace {
constexpr int kMaxRepetitions = 5;  // highest value of the UI repetitions button
}

int timelineScore(const std::vector<TimelineCommand> &commands, const TimelineScoreConfig &config)
{
    int total = 0;
    std::set<TimelineItemType> types;

    const size_t count = std::min(commands.size(), static_cast<size_t>(config.maxSteps));
    for (size_t i = 0; i < count; ++i) {
        const auto &cmd = commands[i];
        const int reps = std::clamp(cmd.repetitions, 1, kMaxRepetitions);
        types.insert(cmd.type);

        switch (cmd.type) {
        case TimelineItemType::Movement: {
            int points = config.movementPoints * reps * 100;
            if (cmd.duration == TimelineDuration::Fast) points = points * (100 + config.fastBonusPercent) / 100;
            else if (cmd.duration == TimelineDuration::Slow) points = points * (100 - config.slowPenaltyPercent) / 100;
            total += (points + 50) / 100;  // round to nearest
            break;
        }
        case TimelineItemType::Animation:
            total += config.animationPoints * reps;
            break;
        case TimelineItemType::Mouth:
            total += config.mouthPoints;
            break;
        }
    }

    if (count > 0) total += config.varietyBonus * static_cast<int>(types.size());
    return total;
}

bool timelineQualifiesForRanking(const std::vector<TimelineCommand> &commands, const TimelineScoreConfig &config)
{
    return static_cast<int>(commands.size()) >= config.minSteps;
}

int timelineMaxPlausibleScore(const TimelineScoreConfig &config)
{
    const int perStep = (config.movementPoints * kMaxRepetitions * (100 + config.fastBonusPercent) + 50) / 100;
    return perStep * config.maxSteps + config.varietyBonus * 3;
}

} // namespace zowi
