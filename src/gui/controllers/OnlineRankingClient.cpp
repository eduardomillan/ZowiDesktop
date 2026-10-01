#include "OnlineRankingClient.h"

#include "RankingController.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVariantMap>

namespace {
constexpr int kTimeoutMs = 8000;
constexpr qint64 kMaxBodyBytes = 256 * 1024;
}

OnlineRankingClient::OnlineRankingClient(RankingController *ranking, bool allowed,
                                         const QString &readUrl, const QString &submitUrl,
                                         QObject *parent)
    : QObject(parent)
    , m_ranking(ranking)
    , m_readUrl(readUrl.trimmed())
    , m_submitUrl(submitUrl.trimmed())
{
    m_available = allowed && ranking && ranking->enabled() && isSafeUrl(m_readUrl);
    if (allowed && !readUrl.trimmed().isEmpty() && !isSafeUrl(m_readUrl))
        qWarning() << "[RankingOnline] Ignoring unsafe ranking_online_read_url:" << readUrl;
}

bool OnlineRankingClient::isSafeUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty() || url.host().isEmpty()) return false;
    if (url.scheme() == QLatin1String("https")) return true;
    if (url.scheme() == QLatin1String("http")) {
        const QString host = url.host();
        return host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1")
               || host == QLatin1String("::1");
    }
    return false;
}

void OnlineRankingClient::setState(WorldState state)
{
    m_state = state;
    emit worldChanged();
}

void OnlineRankingClient::refresh()
{
    if (!m_available || m_worldReply) return;

    QNetworkRequest request(m_readUrl);
    request.setTransferTimeout(kTimeoutMs);
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ZowiDesktop"));
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    setState(Loading);
    QNetworkReply *reply = m_nam.get(request);
    m_worldReply = reply;

    // Never read more than the cap, whatever the server sends.
    connect(reply, &QNetworkReply::downloadProgress, this, [reply](qint64 received, qint64 total) {
        if (received > kMaxBodyBytes || total > kMaxBodyBytes) reply->abort();
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleWorldReply(reply); });
}

void OnlineRankingClient::handleWorldReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_worldReply.clear();

    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError || http != 200) {
        qInfo() << "[RankingOnline] World ranking not available:" << reply->errorString() << "HTTP" << http;
        setState(Error);
        return;
    }

    const QByteArray body = reply->read(kMaxBodyBytes + 1);
    if (body.size() > kMaxBodyBytes) { setState(Error); return; }

    m_worldData = zowi::parseWorldRanking(body.toStdString());
    if (!m_worldData.valid) {
        qWarning() << "[RankingOnline] World ranking JSON is not valid";
        setState(Error);
        return;
    }

    const int own = m_ranking->store().onlineNumber();
    m_world.clear();
    int position = 1;
    for (const auto &e : m_worldData.players) {
        QVariantMap row;
        row["position"] = position++;
        row["number"] = e.number;
        row["name"] = m_ranking->playerName(e.number);
        row["total"] = e.total;
        row["own"] = (own != 0 && e.number == own);
        m_world.append(row);
    }
    m_generated = QString::fromStdString(m_worldData.generated);
    setState(Ready);
}
