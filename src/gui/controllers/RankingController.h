#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <zowi/ranking_store.h>

// QML bridge over zowi::RankingStore: one global ranking where every game adds
// to the active player's total (its own ZowiRanking.json file, separate from
// the session). Players are "Player-NNN" (NNN = 100-999). Games are addressed
// by id: "zowi_says", "mouths", "timeline".
class RankingController : public QObject
{
    Q_OBJECT

public:
    // `enabled` comes from the `ranking_enabled` config key: when false the
    // ranking is hidden everywhere and nothing is recorded.
    explicit RankingController(bool enabled = true, QObject *parent = nullptr);

    Q_PROPERTY(bool enabled READ enabled CONSTANT)
    bool enabled() const { return m_enabled; }

    // Shared with OnlineRankingClient: one in-memory copy of the ranking file.
    zowi::RankingStore &store() { return m_store; }

    // Result codes of createPlayer().
    enum CreateResult { Created = 0, InvalidNumber = 1, NumberTaken = 2 };
    Q_ENUM(CreateResult)

    // [{ position, number, name, total, active }], best first. Always includes
    // the active player (appended with its real position when outside the top).
    Q_INVOKABLE QVariantList top() const;
    // Every local player ordered by number: [{ number, name, total, position, active }].
    Q_INVOKABLE QVariantList players() const;

    Q_INVOKABLE int activePlayer() const;          // 0 when none yet
    Q_INVOKABLE QString activePlayerName() const;  // "" when none yet
    Q_INVOKABLE QString playerName(int number) const;  // "Player-123"

    Q_INVOKABLE int suggestNumber() const;         // free random number, 0 if all taken
    Q_INVOKABLE bool isValidNumber(int number) const;
    Q_INVOKABLE bool isNumberFree(int number) const;
    Q_INVOKABLE int createPlayer(int number);      // CreateResult
    Q_INVOKABLE bool setActivePlayer(int number);

    // Records a finished game for the active player (created on demand):
    // { improved, number, name, total, position }. Only an improvement of the
    // player's best in that game changes the ranking.
    Q_INVOKABLE QVariantMap recordScore(const QString &game, int score);

    // Deliberately no clear/delete here: rankings can only be wiped by an
    // admin through the CLI (`zowi_cli ranking clear`).

signals:
    void rankingChanged();
    void activePlayerChanged();

private:
    zowi::RankingStore m_store;
    bool m_enabled = true;

    static bool gameFromId(const QString &id, zowi::RankingGame &out);
    static QVariantMap toMap(const zowi::RankingPlayer &p, int active);
};
