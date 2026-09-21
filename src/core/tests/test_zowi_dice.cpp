#include "zowi/zowi_dice.h"
#include "zowi/protocol.h"
#include "zowi/robot_commands.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

using namespace zowi;

static int failures = 0;

static void check(bool actual, bool expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << (actual ? "true" : "false")
                  << " expected " << (expected ? "true" : "false") << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkEqual(int actual, int expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got " << actual << " expected " << expected << "\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

static void checkString(const std::string& actual, const std::string& expected, const char* label) {
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got '" << actual << "' expected '" << expected << "'\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

// Drives one full replay action through the robot ACK protocol:
//   M → (&&A) → S → (&&F of the move) → (&&A of the stop) → (&&F of the stop)
// After this the round has advanced by exactly one action and the Stop's acks
// have been drained, so nothing leaks into the next move's counters.
static void replayAction(ZowiDiceGame& game) {
    game.nextRobotCommand();   // the move
    game.onSoftwareAck();      // move accepted → the Stop becomes due mid-cycle
    game.nextRobotCommand();   // the Stop
    game.onFinalAck();         // move's cycle done (its &&F)
    game.onSoftwareAck();      // Stop's &&A
    game.onFinalAck();         // Stop's &&F → round advances
}

int main() {
    // Test 1: Initial state
    {
        ZowiDiceGame game;
        check(game.state() == ZowiDiceState::Idle, true, "initial state idle");
        check(game.currentScore() == 0, true, "initial score 0");
        check(game.sequenceLength() == 0, true, "initial sequence length 0");
        check(game.shouldBlockUserInput() == false, true, "initial block input false");
        check(game.progressPercent() == 0, true, "initial progress 0");
    }

    // Test 2: Start game creates sequence of length 1
    {
        ZowiDiceGame game;
        game.startGame();
        check(game.state() == ZowiDiceState::ShowingSequence, true, "state showing after start");
        check(game.sequenceLength() == 1, true, "sequence length 1 after start");
        check(game.shouldBlockUserInput() == true, true, "blocks input during sequence");
    }

    // Test 3: Sequence grows after successful repeat
    {
        ZowiDiceGame game;
        game.startGame();
        auto firstAction = game.zowiSequence()[0];
        
        // Simulate Zowi playing the sequence (full ACK flow per action)
        replayAction(game);
        check(game.state() == ZowiDiceState::WaitingForUser, true, "waiting for user after ack");
        
        // User repeats correctly
        game.onUserAction(firstAction);
        check(game.state() == ZowiDiceState::ShowingSequence, true, "showing sequence after correct repeat");
        check(game.sequenceLength() == 2, true, "sequence grew to 2");
        check(game.currentScore() == 1, true, "score = length - 1");
    }

    // Test 4: Wrong action ends game
    {
        ZowiDiceGame game;
        game.startGame();
        auto firstAction = game.zowiSequence()[0];
        replayAction(game);
        
        // User presses wrong action
        ZowiDiceAction wrongAction = (firstAction == ZowiDiceAction::TiptoeSwing)
            ? ZowiDiceAction::Jump
            : ZowiDiceAction::TiptoeSwing;
        game.onUserAction(wrongAction);
        check(game.state() == ZowiDiceState::GameOver, true, "game over on wrong action");
        check(game.currentScore() == 0, true, "score 0 for length 1 failure");
    }

    // Test 5: Extra action ends game (user presses more buttons than sequence length within same round)
    {
        ZowiDiceGame game;
        game.startGame();
        auto firstAction = game.zowiSequence()[0];
        replayAction(game);
        
        // User presses wrong action first (should end game)
        ZowiDiceAction wrongAction = (firstAction == ZowiDiceAction::TiptoeSwing)
            ? ZowiDiceAction::Jump
            : ZowiDiceAction::TiptoeSwing;
        game.onUserAction(wrongAction);
        check(game.state() == ZowiDiceState::GameOver, true, "game over on wrong action within round");
    }

    // Test 6: Multiple rounds
    {
        ZowiDiceGame game;
        game.startGame();
        
        // Round 1
        auto a1 = game.zowiSequence()[0];
        replayAction(game);
        game.onUserAction(a1);
        
        // Round 2
        auto a2 = game.zowiSequence()[1];
        replayAction(game);
        replayAction(game); // two actions in sequence
        game.onUserAction(a1);
        game.onUserAction(a2);
        
        // Round 3
        check(game.sequenceLength() == 3, true, "sequence length 3 after 2 rounds");
        check(game.currentScore() == 2, true, "score 2 after 2 rounds");
    }

    // Test 7: Progress percent
    {
        ZowiDiceGame game;
        game.startGame(); // length 1
        check(game.progressPercent() == 0, true, "progress 0 at start of showing");
        replayAction(game); // move to waiting
        check(game.progressPercent() == 0, true, "progress 0 at start of waiting");
        auto a1 = game.zowiSequence()[0];
        game.onUserAction(a1);
        // After user completes sequence, state transitions to ShowingSequence with new action
        // Progress should be 0 again for new sequence
        check(game.progressPercent() == 0, true, "progress 0 at start of new sequence");
    }

    // Test 8: Reset clears state
    {
        ZowiDiceGame game;
        game.startGame();
        replayAction(game);
        auto a1 = game.zowiSequence()[0];
        game.onUserAction(a1);
        game.reset();
        check(game.state() == ZowiDiceState::Idle, true, "reset to idle");
        check(game.sequenceLength() == 0, true, "reset clears sequence");
        check(game.currentScore() == 0, true, "reset clears score");
    }

    // Test 9: Command strings are valid
    {
        ZowiDiceGame game;
        game.startGame();
        std::string cmd = game.nextRobotCommand();
        check(cmd.size() > 0, true, "first command non-empty");
        check(cmd[0] == 'M', true, "first command is Move");
        
        // The Stop is NOT due until the move has been acknowledged (&&A): one
        // pull right after the move must return nothing.
        check(game.nextRobotCommand() == "", true, "no stop before move &&A");
        
        // Once the move is acked, the very next pull is the Stop.
        game.onSoftwareAck();
        std::string stopCmd = game.nextRobotCommand();
        check(stopCmd == "S\r", true, "stop right after move &&A");
        // And the Stop is handed out only once.
        check(game.nextRobotCommand() == "", true, "stop handed out only once");
    }

    // Test 10: Score calculation - play single game to score 12
    {
        ZowiDiceGame game;
        game.startGame();
        // Play 12 rounds (score will be 12)
        // After N rounds: sequence length = N+1, score = N
        for (int round = 1; round <= 12; ++round) {
            // Zowi plays sequence
            for (int j = 0; j < round; ++j) {
                replayAction(game);
            }
            // User repeats
            for (int j = 0; j < round; ++j) {
                game.onUserAction(game.zowiSequence()[j]);
            }
        }
        check(game.currentScore() == 12, true, "score 12 triggers achievement");
    }

    // Test 11: nextRobotCommand interleaves move + stop per action (controller flow)
    {
        ZowiDiceGame game;
        game.startGame(); // round 1: 1 action
        auto action1 = game.zowiSequence()[0];

        // Round 1 playback: move → (&&A) → stop → (move &&F) → (stop &&A) →
        // (stop &&F) advances. The move's &&F must NOT advance by itself.
        std::string c1 = game.nextRobotCommand();
        check((!c1.empty() && c1[0] == 'M'), true, "r1 first command is move");
        game.onSoftwareAck(); // move accepted → stop due mid-cycle
        std::string s1 = game.nextRobotCommand();
        check(s1 == "S\r", true, "r1 stop right after move &&A");
        game.onFinalAck(); // move's &&F (cycle done) → still showing, no advance
        check(game.state() == ZowiDiceState::ShowingSequence, true, "r1 move &&F does not advance");
        check(game.nextRobotCommand() == "", true, "r1 nothing to send while draining stop");
        game.onSoftwareAck(); // stop's &&A
        check(game.nextRobotCommand() == "", true, "r1 nothing to send after stop ack");
        game.onFinalAck(); // stop's &&F → advance → waiting for user
        check(game.state() == ZowiDiceState::WaitingForUser, true, "r1 waiting for user");

        // User repeats correctly -> round 2 with 2 actions
        game.onUserAction(action1);
        check(game.sequenceLength() == 2, true, "r2 sequence length 2");

        // Round 2 playback must show BOTH actions: move, stop, move, stop (with
        // the full ack chain per action).
        std::string c2 = game.nextRobotCommand();
        check((!c2.empty() && c2[0] == 'M'), true, "r2 writes move for first action");
        game.onSoftwareAck();
        std::string s2 = game.nextRobotCommand();
        check(s2 == "S\r", true, "r2 stop after first move &&A");
        game.onFinalAck(); // move's &&F
        game.onSoftwareAck(); // stop's &&A
        game.onFinalAck(); // stop's &&F → advance to second action, keep showing
        check(game.state() == ZowiDiceState::ShowingSequence, true, "r2 still showing after first action");
        std::string c3 = game.nextRobotCommand();
        check((!c3.empty() && c3[0] == 'M'), true, "r2 writes move for second action");
        game.onSoftwareAck();
        std::string s3 = game.nextRobotCommand();
        check(s3 == "S\r", true, "r2 stop after second move &&A");
        game.onFinalAck(); // move's &&F
        game.onSoftwareAck(); // stop's &&A
        game.onFinalAck(); // stop's &&F → advance → waiting for user
        check(game.state() == ZowiDiceState::WaitingForUser, true, "r2 waiting for user");
    }

    // Test 12: currentStep tracks replay and user progress ("X / Y" readout)
    {
        ZowiDiceGame game;
        game.startGame(); // round 1: 1 action
        check(game.currentStep() == 0, true, "currentStep 0 before replay");

        // Replay: move/stop chain → advance to user turn
        game.nextRobotCommand();
        game.onSoftwareAck(); // move accepted
        check(game.currentStep() == 0, true, "currentStep stays 0 during first action");
        game.nextRobotCommand();
        game.onFinalAck(); // move's &&F: must NOT advance
        check(game.currentStep() == 0, true, "currentStep stays 0 at move &&F");
        game.onSoftwareAck();
        game.onFinalAck(); // stop's &&F: advance → waiting for user
        check(game.currentStep() == 0, true, "currentStep 0 at start of user turn");

        // User repeats correctly -> round 2 with 2 actions
        game.onUserAction(game.zowiSequence()[0]);
        check(game.sequenceLength() == 2, true, "round 2 sequence length 2");
        check(game.currentStep() == 0, true, "currentStep 0 at start of round 2 replay");

        // Replay second round: full ACK chain for action 1, then action 2.
        game.nextRobotCommand(); game.onSoftwareAck(); game.nextRobotCommand();
        game.onFinalAck(); game.onSoftwareAck(); game.onFinalAck(); // → index 1
        check(game.currentStep() == 1, true, "currentStep 1 during second replay action");
        game.nextRobotCommand(); game.onSoftwareAck(); game.nextRobotCommand();
        game.onFinalAck(); game.onSoftwareAck(); game.onFinalAck(); // → waiting for user
        check(game.currentStep() == 0, true, "currentStep resets at start of user turn");

        // User presses: step grows with each correct move
        game.onUserAction(game.zowiSequence()[0]);
        check(game.currentStep() == 1, true, "currentStep 1 after first user move");
        game.onUserAction(game.zowiSequence()[1]);
        check(game.currentStep() == 0, true, "currentStep resets on new round");
    }

    // Test 13: TiptoeSwing maps to firmware MoveID 14 (the dice game now uses
    // this move for the top-left action instead of WalkForward/MoveID 1).
    {
        // Same input the game uses: Medium speed (initialSpeedMs=1000) and the
        // default move size 15.
        std::string cmd = commandTiptoeSwing();
        checkString(cmd, "M 14 1000 15\r", "tiptoe-swing command is M 14 1000 15");
    }

    // Test 14: Stale acks are ignored (previous Stop's &&A/&&F must not leak
    // into the next move's counters — the game only advances on the current
    // action's ack chain).
    {
        ZowiDiceGame game;
        game.startGame();

        // A stale &&A arriving before any move is armed is ignored.
        game.onSoftwareAck();
        check(game.state() == ZowiDiceState::ShowingSequence, true, "stale &&A before move does not advance");
        std::string first = game.nextRobotCommand();
        check((!first.empty() && first[0] == 'M'), true, "move still due after stale &&A");

        // A stale &&F before the move's &&A is ignored too (would be the
        // previous Stop's &&F racing the new move).
        game.onFinalAck();
        game.onSoftwareAck(); // real move &&A → stop due
        std::string stop = game.nextRobotCommand();
        check(stop == "S\r", true, "stop still due after stale &&F");

        // Extra &&A while the movement runs does not re-arm the stop.
        game.onSoftwareAck();
        check(game.nextRobotCommand() == "", true, "no double stop after extra &&A");

        // A stray &&F after the round is over leaves the state untouched.
        game.onFinalAck(); game.onSoftwareAck(); game.onFinalAck(); // finish round
        check(game.state() == ZowiDiceState::WaitingForUser, true, "round done via stop &&F");
        game.onFinalAck();
        game.onSoftwareAck();
        check(game.state() == ZowiDiceState::WaitingForUser, true, "stale acks after round are ignored");
    }

    if (failures == 0) {
        std::cout << "\nAll zowi_dice tests passed.\n";
        return 0;
    }
    std::cerr << "\n" << failures << " zowi_dice test(s) failed.\n";
    return 1;
}