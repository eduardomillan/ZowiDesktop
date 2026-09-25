#include "zowi/timeline_command.h"
#include <nlohmann/json.hpp>

namespace zowi {

using json = nlohmann::json;

std::string typeToString(TimelineItemType type) {
    switch (type) {
        case TimelineItemType::Movement: return "movement";
        case TimelineItemType::Animation: return "animation";
        case TimelineItemType::Mouth: return "mouth";
    }
    return "movement";
}

TimelineItemType stringToType(const std::string& s) {
    if (s == "animation") return TimelineItemType::Animation;
    if (s == "mouth") return TimelineItemType::Mouth;
    return TimelineItemType::Movement;
}

std::string durationToString(TimelineDuration dur) {
    switch (dur) {
        case TimelineDuration::Slow: return "Slow";
        case TimelineDuration::Medium: return "Medium";
        case TimelineDuration::Fast: return "Fast";
    }
    return "Medium";
}

TimelineDuration stringToDuration(const std::string& s) {
    if (s == "Slow") return TimelineDuration::Slow;
    if (s == "Fast") return TimelineDuration::Fast;
    return TimelineDuration::Medium;
}

std::string directionToString(TimelineDirection dir) {
    switch (dir) {
        case TimelineDirection::Left: return "Left";
        case TimelineDirection::Right: return "Right";
        case TimelineDirection::Front: return "Front";
    }
    return "Front";
}

TimelineDirection stringToDirection(const std::string& s) {
    if (s == "Left") return TimelineDirection::Left;
    if (s == "Right") return TimelineDirection::Right;
    return TimelineDirection::Front;
}

std::string serializeTimeline(const std::vector<TimelineCommand>& commands) {
    json arr = json::array();
    for (const auto& cmd : commands) {
        arr.push_back({
            {"type", typeToString(cmd.type)},
            {"name", cmd.name},
            {"reps", cmd.repetitions},
            {"duration", durationToString(cmd.duration)},
            {"direction", directionToString(cmd.direction)}
        });
    }
    return arr.dump();
}

std::vector<TimelineCommand> parseTimeline(const std::string& jsonStr) {
    std::vector<TimelineCommand> result;
    try {
        auto arr = json::parse(jsonStr);
        if (!arr.is_array()) return result;

        for (const auto& obj : arr) {
            TimelineCommand cmd;
            cmd.type = stringToType(obj.value("type", "movement"));
            cmd.name = obj.value("name", "");
            cmd.repetitions = obj.value("reps", 1);
            cmd.duration = stringToDuration(obj.value("duration", "Medium"));
            cmd.direction = stringToDirection(obj.value("direction", "Front"));
            result.push_back(cmd);
        }
    } catch (...) {
        // Malformed JSON: return empty timeline
    }
    return result;
}

} // namespace zowi
