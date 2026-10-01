#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

#include <zowi/ranking_store.h>

// QML bridge over zowi::RankingStore (its own ZowiRanking.json file, separate
// from the session). Games are addressed by id:
// "zowi_says", "mouths", "timeline".
class RankingController : public QObject
{
    Q_OBJECT

public:
    explicit RankingController(QObject *parent = nullptr);

    // [{ position, points, playerName }], best first.
    Q_INVOKABLE QVariantList top(const QString &game) const;
    Q_INVOKABLE bool qualifies(const QString &game, int score) const;
    // Returns the 1-based position, or 0 if the score did not qualify.
    Q_INVOKABLE int submit(const QString &game, const QString &playerName, int score);
    Q_INVOKABLE int best(const QString &game) const;
    // Deliberately no clear/delete here: rankings can only be wiped by an
    // admin through the CLI (`zowi_cli ranking clear`).

signals:
    void rankingChanged();

private:
    zowi::RankingStore m_store;

    static bool gameFromId(const QString &id, zowi::RankingGame &out);
};
