#pragma once

#include <string>
#include <vector>

namespace zowi {

enum class TimelineItemType {
    Movement,
    Animation,
    Mouth
};

enum class TimelineDuration {
    Slow = 2000,    // ms
    Medium = 1000,
    Fast = 700
};

enum class TimelineDirection {
    Left,
    Right,
    Front
};

struct TimelineCommand {
    TimelineItemType type;
    std::string name;          // display name (e.g. "Walk Forward", "Happy")
    int repetitions = 1;       // how many times to repeat this command
    TimelineDuration duration = TimelineDuration::Medium;
    TimelineDirection direction = TimelineDirection::Front;

    TimelineCommand() = default;
    TimelineCommand(TimelineItemType t, const std::string& n, int reps = 1,
                    TimelineDuration dur = TimelineDuration::Medium,
                    TimelineDirection dir = TimelineDirection::Front)
        : type(t), name(n), repetitions(reps), duration(dur), direction(dir) {}
};

// Serialize timeline to JSON string (for persistence).
std::string serializeTimeline(const std::vector<TimelineCommand>& commands);

// Deserialize timeline from JSON string.
std::vector<TimelineCommand> parseTimeline(const std::string& json);

} // namespace zowi
