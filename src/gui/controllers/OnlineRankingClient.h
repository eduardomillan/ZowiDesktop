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
class SessionController;

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

    // Sharing (opt-in). `sharing` is the user's choice, kept in the session
    // ("ranking_share_online", off by default).
    Q_PROPERTY(bool canShare READ canShare CONSTANT)
    Q_PROPERTY(bool sharing READ sharing WRITE setSharing NOTIFY shareChanged)
    Q_PROPERTY(int shareState READ shareState NOTIFY shareChanged)
    Q_PROPERTY(bool registered READ registered NOTIFY shareChanged)
    Q_PROPERTY(QString registeredName READ registeredName NOTIFY shareChanged)
    Q_PROPERTY(int sharePosition READ sharePosition NOTIFY shareChanged)
    Q_PROPERTY(QString takenName READ takenName NOTIFY shareChanged)

public:
    enum WorldState { Idle = 0, Loading = 1, Ready = 2, Error = 3 };
    Q_ENUM(WorldState)

    enum ShareState {
        ShareIdle = 0,
        Sending = 1,
        Shared = 2,        // registered / up to date
        Taken = 3,         // the number belongs to someone else online
        NotQualified = 4,  // no points yet, or not enough for the top 100
        RateLimited = 5,
        ShareError = 6     // no connection or the server failed
    };
    Q_ENUM(ShareState)

    // `allowed` comes from `ranking_online_allowed` (and `ranking_enabled`); the
    // URLs from `ranking_online_read_url` / `ranking_online_submit_url`.
    OnlineRankingClient(RankingController *ranking, bool allowed, const QString &readUrl,
                        const QString &submitUrl, QObject *parent = nullptr);

    // Used to persist the sharing choice. Optional (without it, nothing is saved).
    void setSession(SessionController *session);

    // True when the system allows it and the read URL is a safe, usable URL.
    bool available() const { return m_available; }
    int worldState() const { return m_state; }
    QVariantList world() const { return m_world; }
    QString generated() const { return m_generated; }

    // Sharing needs the submit URL on top of `available`.
    bool canShare() const { return m_canShare; }
    bool sharing() const { return m_sharing; }
    void setSharing(bool on);
    int shareState() const { return m_shareState; }
    bool registered() const;
    QString registeredName() const;
    int sharePosition() const { return m_sharePosition; }
    QString takenName() const;

    // Sends the active player's bests when sharing is on (no-op otherwise).
    Q_INVOKABLE void submitNow();
    // "Taken" flow: moves the local player to a free number (keeps the points) and resends.
    Q_INVOKABLE void changeTakenNumber();
    // Removes the online entry now (needs the secret code) and stops sharing.
    Q_INVOKABLE void deleteEntry();

    // Downloads the world ranking (ignored while a download is running).
    Q_INVOKABLE void refresh();

    // https, or http only for localhost (development).
    static bool isSafeUrl(const QUrl &url);

signals:
    void worldChanged();
    void shareChanged();

private:
    void setState(WorldState state);
    void handleWorldReply(QNetworkReply *reply);
    void setShareState(ShareState state);
    QNetworkReply *postJson(const QString &endpoint, const std::string &body);
    void handleSubmitReply(QNetworkReply *reply, int number, bool hadToken, bool retried);
    void handleDeleteReply(QNetworkReply *reply);
    QUrl endpointUrl(const QString &endpoint) const;

    RankingController *m_ranking;
    QNetworkAccessManager m_nam;
    QUrl m_readUrl;
    QUrl m_submitUrl;  // base address of the server (endpoints /submit and /delete)
    SessionController *m_session = nullptr;
    bool m_available = false;
    bool m_canShare = false;
    bool m_sharing = false;
    ShareState m_shareState = ShareIdle;
    int m_sharePosition = 0;
    int m_takenNumber = 0;
    int m_lastSentTotal = -1;
    QPointer<QNetworkReply> m_shareReply;
    WorldState m_state = Idle;
    QVariantList m_world;
    QString m_generated;
    zowi::WorldRanking m_worldData;
    QPointer<QNetworkReply> m_worldReply;
};
