#include "RankingController.h"

#include <QDebug>

#include <algorithm>

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

QVariantMap RankingController::toMap(const zowi::RankingPlayer &p, int active)
{
    QVariantMap row;
    row["position"] = p.position;
    row["number"] = p.number;
    row["name"] = QString::fromStdString(zowi::RankingStore::displayName(p.number));
    row["total"] = p.total;
    row["active"] = (p.number == active);
    return row;
}

QVariantList RankingController::top() const
{
    const int active = m_store.activePlayer();
    QVariantList list;
    bool activeShown = false;
    for (const auto &p : m_store.top()) {
        list.append(toMap(p, active));
        activeShown = activeShown || p.number == active;
    }
    if (active != 0 && !activeShown) {
        for (const auto &p : m_store.players()) {
            if (p.number == active) list.append(toMap(p, active));
        }
    }
    return list;
}

QVariantList RankingController::players() const
{
    const int active = m_store.activePlayer();
    auto all = m_store.players();
    std::sort(all.begin(), all.end(), [](const zowi::RankingPlayer &a, const zowi::RankingPlayer &b) {
        return a.number < b.number;
    });
    QVariantList list;
    for (const auto &p : all) list.append(toMap(p, active));
    return list;
}

int RankingController::activePlayer() const
{
    return m_store.activePlayer();
}

QString RankingController::activePlayerName() const
{
    const int active = m_store.activePlayer();
    return active == 0 ? QString() : playerName(active);
}

QString RankingController::playerName(int number) const
{
    return QString::fromStdString(zowi::RankingStore::displayName(number));
}

int RankingController::suggestNumber() const
{
    return m_store.suggestFreeNumber();
}

bool RankingController::isValidNumber(int number) const
{
    return zowi::RankingStore::isValidNumber(number);
}

bool RankingController::isNumberFree(int number) const
{
    return zowi::RankingStore::isValidNumber(number) && !m_store.hasPlayer(number);
}

int RankingController::createPlayer(int number)
{
    if (!zowi::RankingStore::isValidNumber(number)) return InvalidNumber;
    const int previousActive = m_store.activePlayer();
    if (!m_store.createPlayer(number)) return NumberTaken;
    qInfo() << "[Ranking] Created" << playerName(number);
    emit rankingChanged();
    if (m_store.activePlayer() != previousActive) emit activePlayerChanged();
    return Created;
}

bool RankingController::setActivePlayer(int number)
{
    if (!m_store.setActivePlayer(number)) return false;
    emit activePlayerChanged();
    emit rankingChanged();
    return true;
}

QVariantMap RankingController::recordScore(const QString &game, int score)
{
    QVariantMap out;
    out["improved"] = false;
    out["number"] = 0;
    out["name"] = QString();
    out["total"] = 0;
    out["position"] = 0;

    zowi::RankingGame g;
    if (!gameFromId(game, g)) return out;

    const int previousActive = m_store.activePlayer();
    const auto result = m_store.recordScore(g, score);
    out["improved"] = result.improved;
    out["number"] = result.number;
    out["name"] = result.number ? playerName(result.number) : QString();
    out["total"] = result.total;
    out["position"] = result.position;

    if (result.number != previousActive) emit activePlayerChanged();  // auto-created
    if (result.improved || result.number != previousActive) {
        qInfo() << "[Ranking]" << game << "score" << score << "->" << out["name"].toString()
                << "total" << result.total << "position" << result.position;
        emit rankingChanged();
    }
    return out;
}
