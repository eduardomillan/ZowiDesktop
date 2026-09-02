#include "CommandsController.h"

#include <QString>

CommandsController::CommandsController(QObject *parent)
    : QObject(parent)
{
}

// ── Movement commands ───────────────────────────────────────────────────────
QString CommandsController::walkForward(int speed) const
{
    return QString::fromStdString(zowi::commandWalkForward(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::walkBackward(int speed) const
{
    return QString::fromStdString(zowi::commandWalkBackward(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::turnLeft(int speed) const
{
    return QString::fromStdString(zowi::commandTurnLeft(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::turnRight(int speed) const
{
    return QString::fromStdString(zowi::commandTurnRight(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::moonwalkerLeft(int speed) const
{
    return QString::fromStdString(zowi::commandMoonwalkerLeft(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::moonwalkerRight(int speed) const
{
    return QString::fromStdString(zowi::commandMoonwalkerRight(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::updown(int speed, int size) const
{
    return QString::fromStdString(zowi::commandUpDown(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::swing(int speed, int size) const
{
    return QString::fromStdString(zowi::commandSwing(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::crusaitoForward(int speed, int size) const
{
    return QString::fromStdString(zowi::commandCrusaitoForward(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::crusaitoBackward(int speed, int size) const
{
    return QString::fromStdString(zowi::commandCrusaitoBackward(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::jump(int speed) const
{
    return QString::fromStdString(zowi::commandJump(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::flappingLeft(int speed, int size) const
{
    return QString::fromStdString(zowi::commandFlappingLeft(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::flappingRight(int speed, int size) const
{
    return QString::fromStdString(zowi::commandFlappingRight(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::tiptoeSwing(int speed, int size) const
{
    return QString::fromStdString(zowi::commandTiptoeSwing(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::bendForward(int speed) const
{
    return QString::fromStdString(zowi::commandBendForward(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::bendBackward(int speed) const
{
    return QString::fromStdString(zowi::commandBendBackward(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::shakeLegLeft(int speed) const
{
    return QString::fromStdString(zowi::commandShakeLegLeft(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::shakeLegRight(int speed) const
{
    return QString::fromStdString(zowi::commandShakeLegRight(static_cast<zowi::MovementSpeed>(speed)));
}

QString CommandsController::jitter(int speed, int size) const
{
    return QString::fromStdString(zowi::commandJitter(static_cast<zowi::MovementSpeed>(speed), size));
}

QString CommandsController::ascendingTurn(int speed, int size) const
{
    return QString::fromStdString(zowi::commandAscendingTurn(static_cast<zowi::MovementSpeed>(speed), size));
}

// ── Stop ────────────────────────────────────────────────────────────────────
QString CommandsController::stop() const
{
    return QString::fromStdString(zowi::commandStop());
}

// ── Gesture commands ────────────────────────────────────────────────────────
QString CommandsController::gesture(int gestureId) const
{
    return QString::fromStdString(zowi::commandGesture(gestureId));
}

QString CommandsController::gestureById(int enumId) const
{
    return QString::fromStdString(zowi::commandGesture(static_cast<zowi::GestureId>(enumId)));
}

// ── Mouth commands ──────────────────────────────────────────────────────────
QString CommandsController::mouth(unsigned long matrix) const
{
    return QString::fromStdString(zowi::commandMouth(matrix));
}

QString CommandsController::mouthById(int mouthId) const
{
    return QString::fromStdString(zowi::commandMouthById(static_cast<zowi::MouthId>(mouthId)));
}

// ── Melody commands ─────────────────────────────────────────────────────────
QString CommandsController::sing(int melodyId) const
{
    return QString::fromStdString(zowi::commandSing(static_cast<zowi::MelodyId>(melodyId)));
}

// ── Calibration commands ────────────────────────────────────────────────────
QString CommandsController::setTrims(int yl, int yr, int rl, int rr) const
{
    return QString::fromStdString(zowi::commandSetTrims(yl, yr, rl, rr));
}

QString CommandsController::servoAt(int yl, int yr, int rl, int rr) const
{
    return QString::fromStdString(zowi::commandServoAt(yl, yr, rl, rr));
}