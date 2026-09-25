#include "zowi/timeline_command.h"
#include <iostream>

namespace {

int test_count = 0;
int fail_count = 0;

void checkBool(const char* name, bool result) {
    test_count++;
    if (result) {
        std::cout << "ok [" << name << "]\n";
    } else {
        std::cout << "FAIL [" << name << "]\n";
        fail_count++;
    }
}

void checkInt(const char* name, int got, int expected) {
    test_count++;
    if (got == expected) {
        std::cout << "ok [" << name << "]\n";
    } else {
        std::cout << "FAIL [" << name << "] got=" << got << " expected=" << expected << "\n";
        fail_count++;
    }
}

void checkString(const char* name, const std::string& got, const std::string& expected) {
    test_count++;
    if (got == expected) {
        std::cout << "ok [" << name << "]\n";
    } else {
        std::cout << "FAIL [" << name << "] got=" << got << " expected=" << expected << "\n";
        fail_count++;
    }
}

} // namespace

int main() {
    // Test serialize/parse round-trip
    {
        std::vector<zowi::TimelineCommand> original;
        original.push_back(zowi::TimelineCommand(
            zowi::TimelineItemType::Movement, "Walk Forward", 2,
            zowi::TimelineDuration::Slow, zowi::TimelineDirection::Front));
        original.push_back(zowi::TimelineCommand(
            zowi::TimelineItemType::Animation, "Happy", 1,
            zowi::TimelineDuration::Medium, zowi::TimelineDirection::Front));
        original.push_back(zowi::TimelineCommand(
            zowi::TimelineItemType::Mouth, "Smile", 1,
            zowi::TimelineDuration::Medium, zowi::TimelineDirection::Front));

        std::string json = zowi::serializeTimeline(original);
        std::vector<zowi::TimelineCommand> restored = zowi::parseTimeline(json);

        checkInt("round_trip_count", restored.size(), 3);
        checkBool("round_trip_type_0", restored[0].type == zowi::TimelineItemType::Movement);
        checkString("round_trip_name_0", restored[0].name, "Walk Forward");
        checkInt("round_trip_reps_0", restored[0].repetitions, 2);
        checkBool("round_trip_duration_0", restored[0].duration == zowi::TimelineDuration::Slow);

        checkBool("round_trip_type_1", restored[1].type == zowi::TimelineItemType::Animation);
        checkString("round_trip_name_1", restored[1].name, "Happy");
        checkInt("round_trip_reps_1", restored[1].repetitions, 1);

        checkBool("round_trip_type_2", restored[2].type == zowi::TimelineItemType::Mouth);
        checkString("round_trip_name_2", restored[2].name, "Smile");
    }

    // Test empty timeline
    {
        std::vector<zowi::TimelineCommand> empty;
        std::string json = zowi::serializeTimeline(empty);
        auto restored = zowi::parseTimeline(json);
        checkInt("empty_timeline", restored.size(), 0);
    }

    // Test malformed JSON
    {
        auto restored = zowi::parseTimeline("not valid json");
        checkInt("malformed_json", restored.size(), 0);
    }

    // Test defaults
    {
        zowi::TimelineCommand cmd(zowi::TimelineItemType::Movement, "Test");
        checkInt("default_reps", cmd.repetitions, 1);
        checkBool("default_duration", cmd.duration == zowi::TimelineDuration::Medium);
    }

    std::cout << "\n" << test_count << " tests, " << fail_count << " failures\n";
    return fail_count;
}
