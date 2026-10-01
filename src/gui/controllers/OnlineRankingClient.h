#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QVariantList>

#include <zowi/ranking_online.h>

class QNetworkReply;
class RankingController;

// Optional world ranking. Reads the public ranking JSON (static file on GitHub
// Pages) and, in the sharing part, talks to the small server that registers
// scores. Nothing here makes a request until the user opens the World tab (or,
// later, turns sharing on), and the feature is unavailable unless the system
// allows it and the URLs are configured.
class OnlineRankingClient : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(int worldState READ worldState NOTIFY worldChanged)
    Q_PROPERTY(QVariantList world READ world NOTIFY worldChanged)
    Q_PROPERTY(QString generated READ generated NOTIFY worldChanged)

public:
    enum WorldState { Idle = 0, Loading = 1, Ready = 2, Error = 3 };
    Q_ENUM(WorldState)

    // `allowed` comes from `ranking_online_allowed` (and `ranking_enabled`); the
    // URLs from `ranking_online_read_url` / `ranking_online_submit_url`.
    OnlineRankingClient(RankingController *ranking, bool allowed, const QString &readUrl,
                        const QString &submitUrl, QObject *parent = nullptr);

    // True when the system allows it and the read URL is a safe, usable URL.
    bool available() const { return m_available; }
    int worldState() const { return m_state; }
    QVariantList world() const { return m_world; }
    QString generated() const { return m_generated; }

    // Downloads the world ranking (ignored while a download is running).
    Q_INVOKABLE void refresh();

    // https, or http only for localhost (development).
    static bool isSafeUrl(const QUrl &url);

signals:
    void worldChanged();

private:
    void setState(WorldState state);
    void handleWorldReply(QNetworkReply *reply);

    RankingController *m_ranking;
    QNetworkAccessManager m_nam;
    QUrl m_readUrl;
    QUrl m_submitUrl;
    bool m_available = false;
    WorldState m_state = Idle;
    QVariantList m_world;
    QString m_generated;
    zowi::WorldRanking m_worldData;
    QPointer<QNetworkReply> m_worldReply;
};
