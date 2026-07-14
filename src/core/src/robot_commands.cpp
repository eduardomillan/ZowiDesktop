#include "zowi/robot_commands.h"
#include "zowi/protocol.h"

#include <sstream>

namespace zowi {

namespace {
// Movement command: M <MoveID> <T>\r  (T = period in ms)
std::string buildMovement(int moveId, int periodMs)
{
    return makeCommand(Command::Move,
                        std::to_string(moveId) + ' ' + std::to_string(periodMs));
}
} // namespace

std::string commandWalkForward(MovementSpeed speed)
{
    return buildMovement(1, static_cast<int>(speed));
}

std::string commandWalkBackward(MovementSpeed speed)
{
    return buildMovement(2, static_cast<int>(speed));
}

std::string commandTurnLeft(MovementSpeed speed)
{
    return buildMovement(3, static_cast<int>(speed));
}

std::string commandTurnRight(MovementSpeed speed)
{
    return buildMovement(4, static_cast<int>(speed));
}

std::string commandStop()
{
    return makeCommand(Command::Stop);
}

} // namespace zowi
