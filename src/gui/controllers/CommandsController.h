#ifndef ZOWI_COMMANDS_CONTROLLER_H
#define ZOWI_COMMANDS_CONTROLLER_H

#include <QObject>
#include <QString>

#include "zowi/robot_commands.h"

// Thin Qt adapter that exposes the shared zowi::robot_commands to QML.
// Provides Q_INVOKABLE methods for building firmware command strings
// (movements, gestures, mouths, melodies, calibration, etc.) so the UI
// can send them via Robot.sendData().
//
// Enums are exposed as Q_PROPERTY constants (like RobotController does
// with TransportAuto) because QML can't reference Q_ENUM members from
// context-property objects directly.
class CommandsController : public QObject
{
    Q_OBJECT

    // Movement speed constants (period in ms: larger = slower).
    Q_PROPERTY(int SpeedSlow READ speedSlow CONSTANT)
    Q_PROPERTY(int SpeedMedium READ speedMedium CONSTANT)
    Q_PROPERTY(int SpeedFast READ speedFast CONSTANT)

    // Gesture IDs (0-based enum; protocol is 1-based).
    Q_PROPERTY(int GestureHappy READ gestureHappy CONSTANT)
    Q_PROPERTY(int GestureSuperHappy READ gestureSuperHappy CONSTANT)
    Q_PROPERTY(int GestureSad READ gestureSad CONSTANT)
    Q_PROPERTY(int GestureSleeping READ gestureSleeping CONSTANT)
    Q_PROPERTY(int GestureFart READ gestureFart CONSTANT)
    Q_PROPERTY(int GestureConfused READ gestureConfused CONSTANT)
    Q_PROPERTY(int GestureLove READ gestureLove CONSTANT)
    Q_PROPERTY(int GestureAngry READ gestureAngry CONSTANT)
    Q_PROPERTY(int GestureFretful READ gestureFretful CONSTANT)
    Q_PROPERTY(int GestureMagic READ gestureMagic CONSTANT)
    Q_PROPERTY(int GestureWave READ gestureWave CONSTANT)
    Q_PROPERTY(int GestureVictory READ gestureVictory CONSTANT)
    Q_PROPERTY(int GestureFail READ gestureFail CONSTANT)

    // Mouth IDs (0-based, protocol is also 0-based for mouths).
    Q_PROPERTY(int MouthSmile READ mouthSmile CONSTANT)
    Q_PROPERTY(int MouthHappyOpen READ mouthHappyOpen CONSTANT)
    Q_PROPERTY(int MouthHappyClosed READ mouthHappyClosed CONSTANT)
    Q_PROPERTY(int MouthHeart READ mouthHeart CONSTANT)
    Q_PROPERTY(int MouthBigSurprise READ mouthBigSurprise CONSTANT)
    Q_PROPERTY(int MouthSmallSurprise READ mouthSmallSurprise CONSTANT)
    Q_PROPERTY(int MouthTongueOut READ mouthTongueOut CONSTANT)
    Q_PROPERTY(int MouthVamp1 READ mouthVamp1 CONSTANT)
    Q_PROPERTY(int MouthVamp2 READ mouthVamp2 CONSTANT)
    Q_PROPERTY(int MouthLineMouth READ mouthLineMouth CONSTANT)
    Q_PROPERTY(int MouthConfused READ mouthConfused CONSTANT)
    Q_PROPERTY(int MouthDiagonal READ mouthDiagonal CONSTANT)
    Q_PROPERTY(int MouthSad READ mouthSad CONSTANT)
    Q_PROPERTY(int MouthSadOpen READ mouthSadOpen CONSTANT)
    Q_PROPERTY(int MouthSadClosed READ mouthSadClosed CONSTANT)
    Q_PROPERTY(int MouthOk READ mouthOk CONSTANT)
    Q_PROPERTY(int MouthX READ mouthX CONSTANT)
    Q_PROPERTY(int MouthInterrogation READ mouthInterrogation CONSTANT)
    Q_PROPERTY(int MouthThunder READ mouthThunder CONSTANT)
    Q_PROPERTY(int MouthCulito READ mouthCulito CONSTANT)
    Q_PROPERTY(int MouthAngry READ mouthAngry CONSTANT)

    // Melody IDs (0-based enum; protocol is 1-based).
    Q_PROPERTY(int MelodyConnection READ melodyConnection CONSTANT)
    Q_PROPERTY(int MelodyDisconnection READ melodyDisconnection CONSTANT)
    Q_PROPERTY(int MelodySurprise READ melodySurprise CONSTANT)
    Q_PROPERTY(int MelodyOhOoh READ melodyOhOoh CONSTANT)
    Q_PROPERTY(int MelodyOhOoh2 READ melodyOhOoh2 CONSTANT)
    Q_PROPERTY(int MelodyCuddly READ melodyCuddly CONSTANT)
    Q_PROPERTY(int MelodySleeping READ melodySleeping CONSTANT)
    Q_PROPERTY(int MelodyHappy READ melodyHappy CONSTANT)
    Q_PROPERTY(int MelodySuperHappy READ melodySuperHappy CONSTANT)
    Q_PROPERTY(int MelodyHappyShort READ melodyHappyShort CONSTANT)
    Q_PROPERTY(int MelodySad READ melodySad CONSTANT)
    Q_PROPERTY(int MelodyConfused READ melodyConfused CONSTANT)
    Q_PROPERTY(int MelodyFart1 READ melodyFart1 CONSTANT)
    Q_PROPERTY(int MelodyFart2 READ melodyFart2 CONSTANT)
    Q_PROPERTY(int MelodyFart3 READ melodyFart3 CONSTANT)
    Q_PROPERTY(int MelodyMode1 READ melodyMode1 CONSTANT)
    Q_PROPERTY(int MelodyMode2 READ melodyMode2 CONSTANT)
    Q_PROPERTY(int MelodyMode3 READ melodyMode3 CONSTANT)
    Q_PROPERTY(int MelodyButtonPushed READ melodyButtonPushed CONSTANT)

public:
    explicit CommandsController(QObject *parent = nullptr);
    ~CommandsController() override = default;

    // Speed constants.
    int speedSlow() const { return static_cast<int>(zowi::MovementSpeed::Slow); }
    int speedMedium() const { return static_cast<int>(zowi::MovementSpeed::Medium); }
    int speedFast() const { return static_cast<int>(zowi::MovementSpeed::Fast); }

    // Gesture constants (0-based enum values).
    int gestureHappy() const { return static_cast<int>(zowi::GestureId::Happy); }
    int gestureSuperHappy() const { return static_cast<int>(zowi::GestureId::SuperHappy); }
    int gestureSad() const { return static_cast<int>(zowi::GestureId::Sad); }
    int gestureSleeping() const { return static_cast<int>(zowi::GestureId::Sleeping); }
    int gestureFart() const { return static_cast<int>(zowi::GestureId::Fart); }
    int gestureConfused() const { return static_cast<int>(zowi::GestureId::Confused); }
    int gestureLove() const { return static_cast<int>(zowi::GestureId::Love); }
    int gestureAngry() const { return static_cast<int>(zowi::GestureId::Angry); }
    int gestureFretful() const { return static_cast<int>(zowi::GestureId::Fretful); }
    int gestureMagic() const { return static_cast<int>(zowi::GestureId::Magic); }
    int gestureWave() const { return static_cast<int>(zowi::GestureId::Wave); }
    int gestureVictory() const { return static_cast<int>(zowi::GestureId::Victory); }
    int gestureFail() const { return static_cast<int>(zowi::GestureId::Fail); }

    // Mouth constants (0-based).
    int mouthSmile() const { return static_cast<int>(zowi::MouthId::Smile); }
    int mouthHappyOpen() const { return static_cast<int>(zowi::MouthId::HappyOpen); }
    int mouthHappyClosed() const { return static_cast<int>(zowi::MouthId::HappyClosed); }
    int mouthHeart() const { return static_cast<int>(zowi::MouthId::Heart); }
    int mouthBigSurprise() const { return static_cast<int>(zowi::MouthId::BigSurprise); }
    int mouthSmallSurprise() const { return static_cast<int>(zowi::MouthId::SmallSurprise); }
    int mouthTongueOut() const { return static_cast<int>(zowi::MouthId::TongueOut); }
    int mouthVamp1() const { return static_cast<int>(zowi::MouthId::Vamp1); }
    int mouthVamp2() const { return static_cast<int>(zowi::MouthId::Vamp2); }
    int mouthLineMouth() const { return static_cast<int>(zowi::MouthId::LineMouth); }
    int mouthConfused() const { return static_cast<int>(zowi::MouthId::Confused); }
    int mouthDiagonal() const { return static_cast<int>(zowi::MouthId::Diagonal); }
    int mouthSad() const { return static_cast<int>(zowi::MouthId::Sad); }
    int mouthSadOpen() const { return static_cast<int>(zowi::MouthId::SadOpen); }
    int mouthSadClosed() const { return static_cast<int>(zowi::MouthId::SadClosed); }
    int mouthOk() const { return static_cast<int>(zowi::MouthId::Ok); }
    int mouthX() const { return static_cast<int>(zowi::MouthId::X); }
    int mouthInterrogation() const { return static_cast<int>(zowi::MouthId::Interrogation); }
    int mouthThunder() const { return static_cast<int>(zowi::MouthId::Thunder); }
    int mouthCulito() const { return static_cast<int>(zowi::MouthId::Culito); }
    int mouthAngry() const { return static_cast<int>(zowi::MouthId::Angry); }

    // Melody constants (0-based enum values).
    int melodyConnection() const { return static_cast<int>(zowi::MelodyId::Connection); }
    int melodyDisconnection() const { return static_cast<int>(zowi::MelodyId::Disconnection); }
    int melodySurprise() const { return static_cast<int>(zowi::MelodyId::Surprise); }
    int melodyOhOoh() const { return static_cast<int>(zowi::MelodyId::OhOoh); }
    int melodyOhOoh2() const { return static_cast<int>(zowi::MelodyId::OhOoh2); }
    int melodyCuddly() const { return static_cast<int>(zowi::MelodyId::Cuddly); }
    int melodySleeping() const { return static_cast<int>(zowi::MelodyId::Sleeping); }
    int melodyHappy() const { return static_cast<int>(zowi::MelodyId::Happy); }
    int melodySuperHappy() const { return static_cast<int>(zowi::MelodyId::SuperHappy); }
    int melodyHappyShort() const { return static_cast<int>(zowi::MelodyId::HappyShort); }
    int melodySad() const { return static_cast<int>(zowi::MelodyId::Sad); }
    int melodyConfused() const { return static_cast<int>(zowi::MelodyId::Confused); }
    int melodyFart1() const { return static_cast<int>(zowi::MelodyId::Fart1); }
    int melodyFart2() const { return static_cast<int>(zowi::MelodyId::Fart2); }
    int melodyFart3() const { return static_cast<int>(zowi::MelodyId::Fart3); }
    int melodyMode1() const { return static_cast<int>(zowi::MelodyId::Mode1); }
    int melodyMode2() const { return static_cast<int>(zowi::MelodyId::Mode2); }
    int melodyMode3() const { return static_cast<int>(zowi::MelodyId::Mode3); }
    int melodyButtonPushed() const { return static_cast<int>(zowi::MelodyId::ButtonPushed); }

    // ── Movement commands ───────────────────────────────────────────────────
    Q_INVOKABLE QString walkForward(int speed = 1000) const;
    Q_INVOKABLE QString walkBackward(int speed = 1000) const;
    Q_INVOKABLE QString turnLeft(int speed = 1000) const;
    Q_INVOKABLE QString turnRight(int speed = 1000) const;
    Q_INVOKABLE QString moonwalkerLeft(int speed = 1000) const;
    Q_INVOKABLE QString moonwalkerRight(int speed = 1000) const;
    Q_INVOKABLE QString updown(int speed = 1000, int size = 15) const;
    Q_INVOKABLE QString swing(int speed = 1000, int size = 15) const;
    Q_INVOKABLE QString crusaitoForward(int speed = 1000, int size = 30) const;
    Q_INVOKABLE QString crusaitoBackward(int speed = 1000, int size = 30) const;
    Q_INVOKABLE QString jump(int speed = 1000) const;
    Q_INVOKABLE QString flappingLeft(int speed = 1000, int size = 30) const;
    Q_INVOKABLE QString flappingRight(int speed = 1000, int size = 30) const;
    Q_INVOKABLE QString tiptoeSwing(int speed = 1000, int size = 15) const;
    Q_INVOKABLE QString bendForward(int speed = 1000) const;
    Q_INVOKABLE QString bendBackward(int speed = 1000) const;
    Q_INVOKABLE QString shakeLegLeft(int speed = 1000) const;
    Q_INVOKABLE QString shakeLegRight(int speed = 1000) const;
    Q_INVOKABLE QString jitter(int speed = 1000, int size = 15) const;
    Q_INVOKABLE QString ascendingTurn(int speed = 1000, int size = 15) const;

    // ── Stop ────────────────────────────────────────────────────────────────
    Q_INVOKABLE QString stop() const;

    // ── Gesture commands ────────────────────────────────────────────────────
    // gesture(int) takes the protocol ID (1-based: 1=Happy..13=Fail).
    Q_INVOKABLE QString gesture(int gestureId) const;
    // gestureById(int) takes the enum ID (0-based: 0=Happy..12=Fail) and adds 1.
    Q_INVOKABLE QString gestureById(int enumId) const;

    // ── Mouth commands ──────────────────────────────────────────────────────
    // mouth(unsigned long) takes the raw 32-bit pattern.
    Q_INVOKABLE QString mouth(unsigned long matrix) const;
    // mouthById(int) takes the MouthId enum (0-based) and looks up the pattern.
    Q_INVOKABLE QString mouthById(int mouthId) const;

    // ── Melody commands ─────────────────────────────────────────────────────
    // sing(int) takes the MelodyId enum (0-based) and adds 1 for the protocol.
    Q_INVOKABLE QString sing(int melodyId) const;

    // ── Calibration commands ────────────────────────────────────────────────
    Q_INVOKABLE QString setTrims(int yl, int yr, int rl, int rr) const;
    Q_INVOKABLE QString servoAt(int yl, int yr, int rl, int rr) const;
};

#endif // ZOWI_COMMANDS_CONTROLLER_H