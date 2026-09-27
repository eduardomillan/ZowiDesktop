#include "zowi/timeline_player.h"
#include "zowi/message_parser.h"
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

void checkString(const char* name, const std::string& got, const std::string& expected) {
    test_count++;
    if (got == expected) {
        std::cout << "ok [" << name << "]\n";
    } else {
        std::cout << "FAIL [" << name << "] got='" << got << "' expected='" << expected << "'\n";
        fail_count++;
    }
}

} // namespace

int main() {
    // Test: single movement command (expanded from reps=1)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,   // isMovement
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("single_move_playing", player.isPlaying());

        // Send the movement command
        std::string cmd = player.nextRobotCommand();
        checkBool("single_move_cmd_nonempty", !cmd.empty());
        checkBool("single_move_cmd_is_M", cmd.find('M') != std::string::npos);

        // Simulate &&A (software ack)
        player.onSoftwareAck();
        checkBool("single_move_still_playing_after_ack", player.isPlaying());

        // Simulate &&F (final ack) — should advance to next step
        player.onFinalAck();
        checkBool("single_move_advanced", player.currentIndex() == 1);
        checkBool("single_move_finished", player.finished());
    }

    // Test: single animation command
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandGesture(zowi::GestureId::Happy),
            false,  // isMovement = false
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("animation_playing", player.isPlaying());

        // Send the animation command
        std::string cmd = player.nextRobotCommand();
        checkBool("animation_cmd_nonempty", !cmd.empty());
        checkBool("animation_cmd_is_H", cmd.find('H') != std::string::npos);

        // Simulate &&F (firmware ack for animation)
        player.onFinalAck();
        checkBool("animation_waiting_display", player.isPlaying());

        // UI display timer fires: advanceNonmovement()
        player.advanceNonmovement();
        checkBool("animation_advanced", player.currentIndex() == 1);
        checkBool("animation_finished", player.finished());
    }

    // Test: cancel mid-sequence
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("cancel_start_playing", player.isPlaying());

        // Send command
        player.nextRobotCommand();

        // Cancel
        player.cancel();
        checkBool("cancel_not_playing", !player.isPlaying());
        checkBool("cancel_idle_state", player.state() == zowi::TimelinePlayerState::Idle);
    }

    // Test: sequence of expanded commands (simulating reps)
    // User selected Walk Forward 3 times → expands to 3 separate M commands
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        // Expanded: 3 separate movement commands (from reps=3)
        for (int i = 0; i < 3; ++i) {
            steps.push_back({
                zowi::commandWalkForward(zowi::MovementSpeed::Medium),
                true,
                zowi::MovementSpeed::Medium
            });
        }

        player.start(steps);
        checkBool("reps3_start_playing", player.isPlaying());
        checkBool("reps3_start_index_0", player.currentIndex() == 0);

        // Rep 1: M -> &&A -> &&F -> advance
        std::string cmd1 = player.nextRobotCommand();
        checkBool("reps3_rep1_is_M", !cmd1.empty() && cmd1.find('M') != std::string::npos);
        checkBool("reps3_rep1_still_index_0", player.currentIndex() == 0);

        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("reps3_after_rep1_advanced", player.currentIndex() == 1);

        // Rep 2: M -> &&A -> &&F -> advance
        std::string cmd2 = player.nextRobotCommand();
        checkBool("reps3_rep2_is_M", !cmd2.empty() && cmd2.find('M') != std::string::npos);
        checkBool("reps3_rep2_still_index_1", player.currentIndex() == 1);

        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("reps3_after_rep2_advanced", player.currentIndex() == 2);

        // Rep 3: M -> &&A -> &&F -> advance to finished
        std::string cmd3 = player.nextRobotCommand();
        checkBool("reps3_rep3_is_M", !cmd3.empty() && cmd3.find('M') != std::string::npos);

        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("reps3_all_done", player.currentIndex() == 3);
        checkBool("reps3_finished", player.finished());
    }

    // Test: gesture repeated 3 times (expanded)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        for (int i = 0; i < 3; ++i) {
            steps.push_back({
                zowi::commandGesture(zowi::GestureId::Confused),
                false,
                zowi::MovementSpeed::Medium
            });
        }

        player.start(steps);

        for (int rep = 1; rep <= 3; ++rep) {
            std::string cmd = player.nextRobotCommand();
            checkBool(("gesture_reps3_rep" + std::to_string(rep) + "_is_H").c_str(),
                      !cmd.empty() && cmd.find('H') != std::string::npos);
            checkBool(("gesture_reps3_rep" + std::to_string(rep) + "_index_" + std::to_string(rep-1)).c_str(),
                      player.currentIndex() == rep - 1);

            player.onFinalAck();
            player.advanceNonmovement();
        }

        checkBool("gesture_reps3_all_done", player.currentIndex() == 3);
        checkBool("gesture_reps3_finished", player.finished());
    }

    // Test: currentChipIndex stays constant across all reps of one chip,
    // then advances exactly once per chip boundary (regression test for
    // GUI outline desync: chips [0,1,2] with reps [1,3,1])
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        // Chip 0: 1 rep (movement)
        steps.push_back({zowi::commandWalkForward(zowi::MovementSpeed::Medium), true, zowi::MovementSpeed::Medium, 0});
        // Chip 1: 3 reps (movement)
        for (int i = 0; i < 3; ++i) {
            steps.push_back({zowi::commandWalkForward(zowi::MovementSpeed::Medium), true, zowi::MovementSpeed::Medium, 1});
        }
        // Chip 2: 1 rep (movement)
        steps.push_back({zowi::commandWalkForward(zowi::MovementSpeed::Medium), true, zowi::MovementSpeed::Medium, 2});

        player.start(steps);
        checkBool("chipidx_start_chip0", player.currentChipIndex() == 0);

        // Chip 0's single rep: M -> &&A -> &&F -> advance to chip 1's first rep
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("chipidx_after_chip0_is_chip1", player.currentChipIndex() == 1);
        checkBool("chipidx_after_chip0_stepindex_is_1", player.currentIndex() == 1);

        // Chip 1, rep 1 of 3: chipIndex must stay 1
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("chipidx_chip1_rep1_still_chip1", player.currentChipIndex() == 1);
        checkBool("chipidx_chip1_rep1_stepindex_is_2", player.currentIndex() == 2);

        // Chip 1, rep 2 of 3: chipIndex must still stay 1 (this is the exact bug scenario)
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("chipidx_chip1_rep2_still_chip1", player.currentChipIndex() == 1);
        checkBool("chipidx_chip1_rep2_stepindex_is_3", player.currentIndex() == 3);

        // Chip 1, rep 3 of 3: after this, advance into chip 2
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("chipidx_after_chip1_is_chip2", player.currentChipIndex() == 2);
        checkBool("chipidx_after_chip1_stepindex_is_4", player.currentIndex() == 4);

        // Chip 2's single rep: finishes the sequence
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.onFinalAck();
        checkBool("chipidx_finished_stepindex_is_5", player.currentIndex() == 5);
        checkBool("chipidx_finished_returns_neg1", player.currentChipIndex() == -1);
        checkBool("chipidx_finished_state", player.finished());
    }

    std::cout << "\n" << test_count << " tests, " << fail_count << " failures\n";
    return fail_count;
}
