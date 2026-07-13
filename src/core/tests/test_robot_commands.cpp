#include "zowi/robot_commands.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace zowi;

static int failures = 0;

static void check(const std::string &actual, const std::string &expected, const char *label)
{
    if (actual != expected) {
        std::cerr << "FAIL [" << label << "]: got '" << actual
                  << "' expected '" << expected << "'\n";
        ++failures;
    } else {
        std::cout << "ok   [" << label << "]\n";
    }
}

int main()
{
    check(commandWalkForward(), "M 1 1000\r", "walkForward default");
    check(commandWalkBackward(), "M 2 1000\r", "walkBackward default");
    check(commandTurnLeft(), "M 3 1000\r", "turnLeft default");
    check(commandTurnRight(), "M 4 1000\r", "turnRight default");

    check(commandWalkForward(MovementSpeed::Slow), "M 1 1600\r", "walkForward slow");
    check(commandWalkForward(MovementSpeed::Fast), "M 1 600\r", "walkForward fast");
    check(commandWalkBackward(MovementSpeed::Slow), "M 2 1600\r", "walkBackward slow");
    check(commandTurnLeft(MovementSpeed::Fast), "M 3 600\r", "turnLeft fast");
    check(commandTurnRight(MovementSpeed::Medium), "M 4 1000\r", "turnRight medium");

    check(commandStop(), "S\r", "stop");

    if (failures == 0) {
        std::cout << "All robot_commands tests passed.\n";
        return 0;
    }
    std::cerr << failures << " robot_commands test(s) failed.\n";
    return 1;
}
