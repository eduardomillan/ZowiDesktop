#include "RankingController.h"

#include <QDebug>
#include <QVariantMap>

RankingController::RankingController(QObject *parent)
    : QObject(parent)
{
}

bool RankingController::gameFromId(const QString &id, zowi::RankingGame &out)
{
    if (id == QLatin1String("zowi_says")) out = zowi::RankingGame::ZowiSays;
    else if (id == QLatin1String("mouths")) out = zowi::RankingGame::Mouths;
    else if (id == QLatin1String("timeline")) out = zowi::RankingGame::Timeline;
    else {
        qWarning() << "[Ranking] Unknown game id:" << id;
        return false;
    }
    return true;
}

QVariantList RankingController::top(const QString &game) const
{
    QVariantList list;
    zowi::RankingGame g;
    if (!gameFromId(game, g)) return list;

    int position = 1;
    for (const auto &e : m_store.top(g)) {
        QVariantMap row;
        row["position"] = position++;
        row["points"] = e.points;
        row["playerName"] = QString::fromStdString(e.playerName);
        list.append(row);
    }
    return list;
}

bool RankingController::qualifies(const QString &game, int score) const
{
    zowi::RankingGame g;
    return gameFromId(game, g) && m_store.qualifies(g, score);
}

int RankingController::submit(const QString &game, const QString &playerName, int score)
{
    zowi::RankingGame g;
    if (!gameFromId(game, g)) return 0;
    const int position = m_store.submit(g, playerName.toStdString(), score);
    if (position > 0) {
        qInfo() << "[Ranking]" << game << "score" << score << "entered at position" << position;
        emit rankingChanged();
    }
    return position;
}

int RankingController::best(const QString &game) const
{
    zowi::RankingGame g;
    return gameFromId(game, g) ? m_store.best(g) : 0;
}
