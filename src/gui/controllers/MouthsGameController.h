#pragma once

#include <QObject>
#include <QTimer>
#include <memory>

#include <zowi/mouths_game.h>

class SessionController;

// Thin Qt adapter over zowi::MouthsGame (Qt-free core), exposed to QML as the
// context property "Mouths". Owns the round countdown (QTimer on the GUI
// thread) and the robot cosmetics: target mouth on round start, VICTORY + Stop
// on a solved round, ANGRY + Stop on timeout. Game logic never requires a
// connected robot — commands are simply not sent when disconnected (offline
// note in SCREEN_GAME_MOUTHS.md).
class MouthsGameController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int state READ state NOTIFY stateChanged)
    Q_PROPERTY(int level READ level NOTIFY levelChanged)
    Q_PROPERTY(int score READ score NOTIFY scoreChanged)
    // Target for the current round: MouthId (for the on-screen miniature) and
    // its raw 32-bit pattern (for comparisons).
    Q_PROPERTY(int target READ target NOTIFY targetChanged)
    Q_PROPERTY(int targetPattern READ targetPattern NOTIFY targetChanged)
    // Countdown of the current round: `countdownMs` is what is left right now,
    // `roundTimeMs` is the full duration (the bar works on countdown/round).
    Q_PROPERTY(int countdownMs READ countdownMs NOTIFY countdownMsChanged)
    Q_PROPERTY(int roundTimeMs READ roundTimeMs NOTIFY countdownMsChanged)
    // Named state constants: QML enum lookups like `Mouths.State.Idle` do NOT
    // resolve for context-property instances, so comparisons must use these
    // value properties (e.g. `Mouths.state === Mouths.stateRoundActive`).
    Q_PROPERTY(int stateIdle READ stateIdle CONSTANT)
    Q_PROPERTY(int stateRoundActive READ stateRoundActive CONSTANT)
    Q_PROPERTY(int stateRoundSolved READ stateRoundSolved CONSTANT)
    Q_PROPERTY(int stateGameOver READ stateGameOver CONSTANT)

public:
    enum State {
        Idle = 0,
        RoundActive = 1,
        RoundSolved = 2,
        GameOver = 3
    };
    Q_ENUM(State)

    explicit MouthsGameController(QObject* parent = nullptr);
    ~MouthsGameController();

    void setSessionController(SessionController* session);

    int state() const;
    int level() const;
    int score() const;
    int target() const;
    int targetPattern() const;
    int countdownMs() const;
    int roundTimeMs() const;
    int stateIdle() const { return static_cast<int>(State::Idle); }
    int stateRoundActive() const { return static_cast<int>(State::RoundActive); }
    int stateRoundSolved() const { return static_cast<int>(State::RoundSolved); }
    int stateGameOver() const { return static_cast<int>(State::GameOver); }

    Q_INVOKABLE void startGame();
    Q_INVOKABLE void resetGame();
    // Live draw comparison (mirrors Android's MouthGridLayoutTouchListener →
    // checkLedMouth): `matrix` is the 32-bit mouth pattern built from the 6×5
    // grid (bit 29−i for cell i, like MouthGrid). Returns true when this draw
    // just solved the round.
    Q_INVOKABLE bool submitDraw(unsigned long matrix);

signals:
    void stateChanged();
    void levelChanged();
    void scoreChanged();
    void targetChanged();
    void countdownMsChanged();
    void gameOver(int score);
    // Robot cosmetics are emitted as raw commands so the controller does not
    // depend on the concrete robot type: main.cpp (and the screen preview)
    // forwards them to the active robot, gated by its connection state.
    void sendCommand(const QString &data);

private:
    void startRound();
    void onCountdownTick();
    void onVictoryPauseElapsed();
    void sendTargetMouth();
    void sendVictory();
    void sendAngryAndStop();
    void updateFromGame();
    void saveLastScore();

    std::unique_ptr<zowi::MouthsGame> m_game;
    SessionController* m_session = nullptr;

    // Round countdown: ticks every 100 ms while a round is active.
    QTimer m_countdown;
    // Brief VICTORY intermission between a solved round and the next one.
    QTimer m_victoryPause;
    int m_countdownMs = 0;
    int m_roundTimeMs = 0;
};