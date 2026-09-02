#ifndef ZOWI_CALIBRATION_SESSION_CONTROLLER_H
#define ZOWI_CALIBRATION_SESSION_CONTROLLER_H

#include <QObject>

#include "zowi/calibration_session.h"

// Thin Qt adapter that exposes the shared zowi::CalibrationSession domain model
// to QML. Owns the debounce/"one G in flight" policy sanctioned by the core
// session and emits sendCommand() for the live servosCommand() that main.cpp
// wires to RobotController::sendData. Reactive properties (trims + step) let
// QML bind straight to them.
class CalibrationSessionController : public QObject
{
    Q_OBJECT

    // Trim offsets indexed like the servo order YL YR RL RR.
    Q_PROPERTY(int trimYL READ trim0 NOTIFY trimsChanged)
    Q_PROPERTY(int trimYR READ trim1 NOTIFY trimsChanged)
    Q_PROPERTY(int trimRL READ trim2 NOTIFY trimsChanged)
    Q_PROPERTY(int trimRR READ trim3 NOTIFY trimsChanged)
    Q_PROPERTY(int step READ stepIndex NOTIFY stepChanged)

public:
    explicit CalibrationSessionController(QObject *parent = nullptr);
    ~CalibrationSessionController() override = default;

    int trim0() const;
    int trim1() const;
    int trim2() const;
    int trim3() const;
    int stepIndex() const;

    Q_INVOKABLE bool adjust(int index, int delta);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void nextStep();
    Q_INVOKABLE void jumpToStep(int stepIndex);

    // Resets trims and restarts the send clock, returning the neutral command
    // to send so the robot returns to 90°.
    Q_INVOKABLE QString resetToNeutral();

    // Applies the debounce policy; emits sendCommand(servosCommand()) when it
    // is time to move, returns true if a send was issued (false = coalesced).
    Q_INVOKABLE bool sendServos();

    // The persisted-trims (C) command, for the caller to forward on save/test.
    Q_INVOKABLE QString trimsCommand() const;

signals:
    void trimsChanged();
    void sendCommand(const QString &data);
    void stepChanged();

private:
    zowi::CalibrationSession m_session;
    long long m_lastSendMs = -1;
};

#endif // ZOWI_CALIBRATION_SESSION_CONTROLLER_H