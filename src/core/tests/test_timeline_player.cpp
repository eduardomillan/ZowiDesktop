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
    // Test: single movement with default cycles=1
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,   // isMovement
            zowi::MovementSpeed::Medium
            // cycles defaults to 1
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

        // For cycles=1, Stop is sent immediately after &&A
        std::string stopCmd = player.nextRobotCommand();
        checkBool("single_move_stop_sent_after_ack", !stopCmd.empty() && stopCmd[0] == 'S');

        // Simulate &&F (final ack) — should now await advance
        player.onFinalAck();
        checkBool("single_move_awaiting_advance", player.movementAwaitingAdvance());

        // Advance it
        player.advanceMovement();
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

    // Test: movement with cycles=3 (one step, three gait cycles, no per-cycle stutter)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,
            zowi::MovementSpeed::Medium,
            3  // cycles=3
        });
        steps.push_back({
            zowi::commandGesture(zowi::GestureId::Sad),
            false,
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("cycles3_start_playing", player.isPlaying());
        checkBool("cycles3_start_index_0", player.currentIndex() == 0);

        // Send M once
        std::string cmd1 = player.nextRobotCommand();
        checkBool("cycles3_cmd1_is_M", !cmd1.empty() && cmd1.find('M') != std::string::npos);
        checkBool("cycles3_still_index_0_after_M", player.currentIndex() == 0);

        // Second call (before any ack) returns nothing
        std::string cmd1b = player.nextRobotCommand();
        checkBool("cycles3_no_resend_before_ack", cmd1b.empty());

        // Ack 1: &&A
        player.onSoftwareAck();
        checkBool("cycles3_not_awaiting_after_ack1", !player.movementAwaitingAdvance());

        // Ack 1: &&F (cycle 1 complete)
        player.onFinalAck();
        checkBool("cycles3_not_awaiting_after_f1", !player.movementAwaitingAdvance());
        // Stop not sent yet (would be after ack N-1 = ack 2)
        std::string stopCheck1 = player.nextRobotCommand();
        checkBool("cycles3_no_stop_yet_after_f1", stopCheck1.empty());

        // Ack 2: &&A (start of cycle 2)
        player.onSoftwareAck();

        // Ack 2: &&F (cycle 2 complete = ack N-1)
        player.onFinalAck();
        // Now Stop should be queued and returned
        std::string stopCmd = player.nextRobotCommand();
        checkBool("cycles3_stop_sent_after_f2", !stopCmd.empty() && stopCmd[0] == 'S');

        // Ack 3: &&A (start of cycle 3)
        player.onSoftwareAck();

        // Ack 3: &&F (cycle 3 complete = ack N)
        player.onFinalAck();
        checkBool("cycles3_awaiting_after_f3", player.movementAwaitingAdvance());
        checkBool("cycles3_still_index_0_awaiting", player.currentIndex() == 0);

        // Advance it (simulating the stop-ack drain completing)
        player.advanceMovement();
        checkBool("cycles3_advanced_to_index1", player.currentIndex() == 1);

        // Next step should be the gesture
        std::string nextCmd = player.nextRobotCommand();
        checkBool("cycles3_next_is_gesture_H", !nextCmd.empty() && nextCmd.find('H') != std::string::npos);
    }

    // Test: gesture repeated 3 times (expanded into 3 separate H commands)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        for (int i = 0; i < 3; ++i) {
            steps.push_back({
                zowi::commandGesture(zowi::GestureId::Confused),
                false,
                zowi::MovementSpeed::Medium,
                1  // each rep is cycles=1 (default)
            });
        }

        player.start(steps);

        for (int rep = 0; rep < 3; ++rep) {
            std::string cmd = player.nextRobotCommand();
            checkBool(("gesture_reps3_rep" + std::to_string(rep) + "_is_H").c_str(),
                      !cmd.empty() && cmd.find('H') != std::string::npos);
            checkBool(("gesture_reps3_rep" + std::to_string(rep) + "_index_" + std::to_string(rep)).c_str(),
                      player.currentIndex() == rep);

            player.onFinalAck();
            player.advanceNonmovement();
        }

        checkBool("gesture_reps3_all_done", player.currentIndex() == 3);
        checkBool("gesture_reps3_finished", player.finished());
    }

    // Test: cancel mid-movement-with-cycles (cycles=3)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,
            zowi::MovementSpeed::Medium,
            3
        });

        player.start(steps);
        player.nextRobotCommand();  // M sent
        player.onSoftwareAck();       // &&A
        player.onFinalAck();          // &&F cycle 1

        // Cancel mid-cycle
        player.cancel();
        checkBool("cancel_mid_cycles_state_idle", player.state() == zowi::TimelinePlayerState::Idle);

        // Start a new step and verify no leftover m_stopSent state
        std::vector<zowi::TimelineStep> newSteps;
        newSteps.push_back({
            zowi::commandWalkForward(zowi::MovementSpeed::Medium),
            true,
            zowi::MovementSpeed::Medium,
            1
        });
        player.start(newSteps);
        std::string newCmd = player.nextRobotCommand();
        checkBool("cancel_mid_cycles_fresh_start_is_M", !newCmd.empty() && newCmd.find('M') != std::string::npos);

        player.onSoftwareAck();
        std::string nextCmd = player.nextRobotCommand();
        checkBool("cancel_mid_cycles_fresh_stop_sent", !nextCmd.empty() && nextCmd[0] == 'S');
    }

    std::cout << "\n" << test_count << " tests, " << fail_count << " failures\n";
    return fail_count;
}
