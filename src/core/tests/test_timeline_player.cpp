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
    // Test: single movement with reps=1 (simplest case)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandTiptoeSwing(zowi::MovementSpeed::Medium),
            true,   // isMovement
            1,      // cycles=1 (reps)
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("single_move_playing", player.isPlaying());

        // First command should be the movement
        std::string cmd1 = player.nextRobotCommand();
        checkBool("single_move_cmd1_nonempty", !cmd1.empty());
        checkBool("single_move_cmd1_is_M", cmd1.find('M') != std::string::npos);

        // Simulate &&A (software ack)
        player.onSoftwareAck();

        // After &&A, should request Stop on next query
        std::string cmd2 = player.nextRobotCommand();
        checkBool("single_move_cmd2_stop", cmd2.find('S') != std::string::npos);

        // Simulate &&F (final ack) for cycle 1 (transitions to AwaitingStopAcks)
        player.onFinalAck();

        // Simulate &&A for the Stop command
        player.onSoftwareAck();

        // Simulate &&F for the Stop (should advance and finish)
        player.onFinalAck();

        // After Stop's final ack, should be finished
        checkBool("single_move_finished", player.finished());
        checkBool("single_move_not_playing", !player.isPlaying());
    }

    // Test: single animation (non-movement)
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandGesture(zowi::GestureId::Happy),
            false,  // not a movement (animation)
            1,      // 1 rep
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("animation_playing", player.isPlaying());

        // Execute animation (gesture)
        std::string gestureCmd = player.nextRobotCommand();
        checkBool("animation_cmd_nonempty", !gestureCmd.empty());
        checkBool("animation_cmd_is_H", gestureCmd.find('H') != std::string::npos);

        player.onSoftwareAck();
        player.onFinalAck();
        // Non-movements now await the display timer firing (simulated by the UI/test)
        // After ack, we're in MotionlessAwaitingDisplay; call advanceNonmovement() as the UI timer would
        player.advanceNonmovement();

        // Now we should be finished (no more steps)
        checkBool("animation_finished", player.finished());
    }

    // Test: cancel mid-sequence
    {
        zowi::TimelinePlayer player;
        std::vector<zowi::TimelineStep> steps;
        steps.push_back({
            zowi::commandJump(zowi::MovementSpeed::Fast),
            true,
            1,
            zowi::MovementSpeed::Fast
        });
        steps.push_back({
            zowi::commandGesture(zowi::GestureId::Sad),
            false,
            1,
            zowi::MovementSpeed::Medium
        });

        player.start(steps);
        checkBool("cancel_start_playing", player.isPlaying());

        // Get first command
        player.nextRobotCommand();
        player.onSoftwareAck();
        player.nextRobotCommand();  // Stop

        // Cancel mid-sequence
        player.cancel();
        checkBool("cancel_not_playing", !player.isPlaying());
        checkBool("cancel_idle_state", player.state() == zowi::TimelinePlayerState::Idle);
    }

    std::cout << "\n" << test_count << " tests, " << fail_count << " failures\n";
    return fail_count;
}
