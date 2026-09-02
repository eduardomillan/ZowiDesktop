#include "CalibrationSessionController.h"

#include <QDateTime>

CalibrationSessionController::CalibrationSessionController(QObject *parent)
    : QObject(parent)
{
}

int CalibrationSessionController::trim0() const
{
    return m_session.trim(zowi::CalibrationSession::LeftLeg);
}

int CalibrationSessionController::trim1() const
{
    return m_session.trim(zowi::CalibrationSession::RightLeg);
}

int CalibrationSessionController::trim2() const
{
    return m_session.trim(zowi::CalibrationSession::LeftFoot);
}

int CalibrationSessionController::trim3() const
{
    return m_session.trim(zowi::CalibrationSession::RightFoot);
}

int CalibrationSessionController::stepIndex() const
{
    return m_session.stepIndex();
}

bool CalibrationSessionController::adjust(int index, int delta)
{
    const bool changed = m_session.adjust(index, delta);
    if (changed)
        emit trimsChanged();
    return changed;
}

void CalibrationSessionController::reset()
{
    m_session.reset();
    m_lastSendMs = -1;
    emit trimsChanged();
    emit stepChanged();
}

void CalibrationSessionController::nextStep()
{
    m_session.nextStep();
    emit stepChanged();
}

void CalibrationSessionController::jumpToStep(int stepIndex)
{
    m_session.setStepIndex(stepIndex);
    emit stepChanged();
}

QString CalibrationSessionController::resetToNeutral()
{
    reset();
    return QStringLiteral("S\r");
}

bool CalibrationSessionController::sendServos()
{
    const long long now = QDateTime::currentMSecsSinceEpoch();
    if (!m_session.shouldSend(now, m_lastSendMs)) {
        return false;
    }
    emit sendCommand(QString::fromStdString(m_session.servosCommand()));
    return true;
}

QString CalibrationSessionController::trimsCommand() const
{
    return QString::fromStdString(m_session.trimsCommand());
}