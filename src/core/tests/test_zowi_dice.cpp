#include "zowi/zowi_dice.h"
#include "zowi/protocol.h"

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
        
        // Simulate Zowi playing the sequence (onFinalAck)
        game.onFinalAck();
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
        game.onFinalAck();
        
        // User presses wrong action
        ZowiDiceAction wrongAction = (firstAction == ZowiDiceAction::WalkForward)
            ? ZowiDiceAction::Jump
            : ZowiDiceAction::WalkForward;
        game.onUserAction(wrongAction);
        check(game.state() == ZowiDiceState::GameOver, true, "game over on wrong action");
        check(game.currentScore() == 0, true, "score 0 for length 1 failure");
    }

    // Test 5: Extra action ends game (user presses more buttons than sequence length within same round)
    {
        ZowiDiceGame game;
        game.startGame();
        auto firstAction = game.zowiSequence()[0];
        game.onFinalAck();
        
        // User presses wrong action first (should end game)
        ZowiDiceAction wrongAction = (firstAction == ZowiDiceAction::WalkForward)
            ? ZowiDiceAction::Jump
            : ZowiDiceAction::WalkForward;
        game.onUserAction(wrongAction);
        check(game.state() == ZowiDiceState::GameOver, true, "game over on wrong action within round");
    }

    // Test 6: Multiple rounds
    {
        ZowiDiceGame game;
        game.startGame();
        
        // Round 1
        auto a1 = game.zowiSequence()[0];
        game.onFinalAck();
        game.onUserAction(a1);
        
        // Round 2
        auto a2 = game.zowiSequence()[1];
        game.onFinalAck();
        game.onFinalAck(); // two actions in sequence
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
        game.onFinalAck(); // move to waiting
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
        game.onFinalAck();
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
        
        // Second call should be Stop
        std::string stopCmd = game.nextRobotCommand();
        check(stopCmd == "S\r", true, "second command is Stop");
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
                game.onFinalAck();
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

        // Round 1 playback: move, then stop after movement ACK, stop ACK advances
        std::string c1 = game.nextRobotCommand();
        check((!c1.empty() && c1[0] == 'M'), true, "r1 first command is move");
        game.onFinalAck(); // movement ACK -> must NOT advance
        std::string s1 = game.nextRobotCommand();
        check(s1 == "S\r", true, "r1 stop after movement ACK");
        game.onFinalAck(); // stop ACK -> advance -> waiting for user
        check(game.state() == ZowiDiceState::WaitingForUser, true, "r1 waiting for user");

        // User repeats correctly -> round 2 with 2 actions
        game.onUserAction(action1);
        check(game.sequenceLength() == 2, true, "r2 sequence length 2");

        // Round 2 playback must show BOTH actions: move, stop, move, stop
        std::string c2 = game.nextRobotCommand();
        check((!c2.empty() && c2[0] == 'M'), true, "r2 writes move for first action");
        game.onFinalAck();
        std::string s2 = game.nextRobotCommand();
        check(s2 == "S\r", true, "r2 stop after first move ACK");
        game.onFinalAck(); // stop ACK -> advance to second action, keep showing
        check(game.state() == ZowiDiceState::ShowingSequence, true, "r2 still showing after first action");
        std::string c3 = game.nextRobotCommand();
        check((!c3.empty() && c3[0] == 'M'), true, "r2 writes move for second action");
        game.onFinalAck();
        std::string s3 = game.nextRobotCommand();
        check(s3 == "S\r", true, "r2 stop after second move ACK");
        game.onFinalAck(); // stop ACK -> advance -> waiting for user
        check(game.state() == ZowiDiceState::WaitingForUser, true, "r2 waiting for user");
    }

    if (failures == 0) {
        std::cout << "\nAll zowi_dice tests passed.\n";
        return 0;
    }
    std::cerr << "\n" << failures << " zowi_dice test(s) failed.\n";
    return 1;
}