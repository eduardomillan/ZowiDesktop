#include <zowi/timeline_score.h>

#include <cassert>
#include <iostream>

using namespace zowi;

static TimelineCommand cmd(TimelineItemType t, int reps = 1, TimelineDuration d = TimelineDuration::Medium) {
    return TimelineCommand(t, "x", reps, d);
}

void test_empty() {
    std::cout << "test_empty: " << std::flush;
    assert(timelineScore({}) == 0);
    assert(!timelineQualifiesForRanking({}));
    std::cout << "OK" << std::endl;
}

void test_per_type_points() {
    std::cout << "test_per_type_points: " << std::flush;
    // movement 3 + variety 2 = 5
    assert(timelineScore({cmd(TimelineItemType::Movement)}) == 5);
    // animation 2 + 2
    assert(timelineScore({cmd(TimelineItemType::Animation)}) == 4);
    // mouth 1 + 2
    assert(timelineScore({cmd(TimelineItemType::Mouth)}) == 3);
    std::cout << "OK" << std::endl;
}

void test_repetitions_and_mouth_ignores_reps() {
    std::cout << "test_repetitions_and_mouth_ignores_reps: " << std::flush;
    assert(timelineScore({cmd(TimelineItemType::Movement, 5)}) == 3 * 5 + 2);
    assert(timelineScore({cmd(TimelineItemType::Animation, 3)}) == 2 * 3 + 2);
    assert(timelineScore({cmd(TimelineItemType::Mouth, 5)}) == 1 + 2);
    // Out-of-range reps are clamped to 1..5
    assert(timelineScore({cmd(TimelineItemType::Movement, 99)}) == 3 * 5 + 2);
    assert(timelineScore({cmd(TimelineItemType::Movement, 0)}) == 3 + 2);
    std::cout << "OK" << std::endl;
}

void test_speed_multiplier_movements_only() {
    std::cout << "test_speed_multiplier_movements_only: " << std::flush;
    // Fast: 3*1.2 = 3.6 -> 4 ; Slow: 3*0.8 = 2.4 -> 2 (+2 variety)
    assert(timelineScore({cmd(TimelineItemType::Movement, 1, TimelineDuration::Fast)}) == 4 + 2);
    assert(timelineScore({cmd(TimelineItemType::Movement, 1, TimelineDuration::Slow)}) == 2 + 2);
    // Speed does not affect animations
    assert(timelineScore({cmd(TimelineItemType::Animation, 1, TimelineDuration::Fast)}) == 2 + 2);
    std::cout << "OK" << std::endl;
}

void test_variety_bonus() {
    std::cout << "test_variety_bonus: " << std::flush;
    const int s = timelineScore({cmd(TimelineItemType::Movement), cmd(TimelineItemType::Animation),
                                 cmd(TimelineItemType::Mouth)});
    assert(s == 3 + 2 + 1 + 3 * 2);
    // Repeating the same type does not add more variety
    assert(timelineScore({cmd(TimelineItemType::Movement), cmd(TimelineItemType::Movement)}) == 3 + 3 + 2);
    std::cout << "OK" << std::endl;
}

void test_step_cap_and_qualification() {
    std::cout << "test_step_cap_and_qualification: " << std::flush;
    std::vector<TimelineCommand> many(100, cmd(TimelineItemType::Movement));
    assert(timelineScore(many) == 40 * 3 + 2);  // only the first 40 count
    assert(timelineScore(many) <= timelineMaxPlausibleScore());

    std::vector<TimelineCommand> four(4, cmd(TimelineItemType::Mouth));
    std::vector<TimelineCommand> five(5, cmd(TimelineItemType::Mouth));
    assert(!timelineQualifiesForRanking(four));
    assert(timelineQualifiesForRanking(five));
    std::cout << "OK" << std::endl;
}

void test_max_plausible_is_upper_bound() {
    std::cout << "test_max_plausible_is_upper_bound: " << std::flush;
    std::vector<TimelineCommand> best(40, cmd(TimelineItemType::Movement, 5, TimelineDuration::Fast));
    best[1] = cmd(TimelineItemType::Animation, 5);
    best[2] = cmd(TimelineItemType::Mouth, 5);
    assert(timelineScore(best) <= timelineMaxPlausibleScore());
    std::cout << "OK" << std::endl;
}

int main() {
    test_empty();
    test_per_type_points();
    test_repetitions_and_mouth_ignores_reps();
    test_speed_multiplier_movements_only();
    test_variety_bonus();
    test_step_cap_and_qualification();
    test_max_plausible_is_upper_bound();
    std::cout << "All timeline_score tests passed." << std::endl;
    return 0;
}
