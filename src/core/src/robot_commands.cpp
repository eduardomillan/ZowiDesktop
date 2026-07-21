#include "zowi/robot_commands.h"
#include "zowi/protocol.h"

#include <sstream>

namespace zowi {

namespace {
// Movement command: M <MoveID> <T> [<MoveSize>]\r
// moveSize is only used by certain MoveIDs (6-14, 19-20); pass 0 to omit.
std::string buildMovement(int moveId, int periodMs, int moveSize = 0)
{
    std::string args = std::to_string(moveId) + ' ' + std::to_string(periodMs);
    if (moveSize > 0) {
        args += ' ' + std::to_string(moveSize);
    }
    return makeCommand(Command::Move, args);
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

std::string commandMoonwalkerLeft(MovementSpeed speed)
{
    return buildMovement(6, static_cast<int>(speed), 30);
}

std::string commandMoonwalkerRight(MovementSpeed speed)
{
    return buildMovement(7, static_cast<int>(speed), 30);
}

std::string commandUpDown(MovementSpeed speed, int size)
{
    return buildMovement(5, static_cast<int>(speed), size);
}

std::string commandSwing(MovementSpeed speed, int size)
{
    return buildMovement(8, static_cast<int>(speed), size);
}

std::string commandCrusaitoForward(MovementSpeed speed, int size)
{
    return buildMovement(9, static_cast<int>(speed), size);
}

std::string commandCrusaitoBackward(MovementSpeed speed, int size)
{
    return buildMovement(10, static_cast<int>(speed), size);
}

std::string commandJump(MovementSpeed speed)
{
    return buildMovement(11, static_cast<int>(speed));
}

std::string commandFlappingLeft(MovementSpeed speed, int size)
{
    return buildMovement(12, static_cast<int>(speed), size);
}

std::string commandFlappingRight(MovementSpeed speed, int size)
{
    return buildMovement(13, static_cast<int>(speed), size);
}

std::string commandTiptoeSwing(MovementSpeed speed, int size)
{
    return buildMovement(14, static_cast<int>(speed), size);
}

std::string commandBendForward(MovementSpeed speed)
{
    return buildMovement(15, static_cast<int>(speed));
}

std::string commandBendBackward(MovementSpeed speed)
{
    return buildMovement(16, static_cast<int>(speed));
}

std::string commandShakeLegLeft(MovementSpeed speed)
{
    return buildMovement(17, static_cast<int>(speed));
}

std::string commandShakeLegRight(MovementSpeed speed)
{
    return buildMovement(18, static_cast<int>(speed));
}

std::string commandJitter(MovementSpeed speed, int size)
{
    return buildMovement(19, static_cast<int>(speed), size);
}

std::string commandAscendingTurn(MovementSpeed speed, int size)
{
    return buildMovement(20, static_cast<int>(speed), size);
}

std::string commandStop()
{
    return makeCommand(Command::Stop);
}

} // namespace zowi
