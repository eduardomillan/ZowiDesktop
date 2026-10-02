#include "OnlineRankingClient.h"

#include "RankingController.h"
#include "SessionController.h"

#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QVariantMap>

#include <algorithm>
#include <vector>

namespace {
constexpr int kTimeoutMs = 8000;
constexpr qint64 kMaxBodyBytes = 256 * 1024;
constexpr qint64 kMaxReplyBytes = 4096;  // replies from the server are tiny
constexpr const char *kShareKey = "ranking_share_online";
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

    m_canShare = m_available && isSafeUrl(m_submitUrl);
    if (m_available && !submitUrl.trimmed().isEmpty() && !m_canShare)
        qWarning() << "[RankingOnline] Ignoring unsafe ranking_online_submit_url:" << submitUrl;

    if (m_canShare) {
        // Send again when a game improves the total, and start fresh for another player.
        connect(ranking, &RankingController::rankingChanged, this, [this]() {
            if (!m_sharing) return;
            const int active = m_ranking->store().activePlayer();
            if (active != 0 && m_ranking->store().total(active) != m_lastSentTotal) submitNow();
        });
        connect(ranking, &RankingController::activePlayerChanged, this, [this]() {
            m_lastSentTotal = -1;
            m_sharePosition = 0;
            if (m_shareState != Sending) m_shareState = ShareIdle;
            emit shareChanged();
        });
    }
}

void OnlineRankingClient::setSession(SessionController *session)
{
    m_session = session;
    if (m_session && m_canShare)
        m_sharing = m_session->getString(kShareKey, QStringLiteral("false")) == QLatin1String("true");
    if (m_sharing) submitNow();
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

// ── Sharing ─────────────────────────────────────────────────────────────

bool OnlineRankingClient::registered() const
{
    return m_ranking->store().onlineNumber() != 0;
}

QString OnlineRankingClient::registeredName() const
{
    const int number = m_ranking->store().onlineNumber();
    return number ? m_ranking->playerName(number) : QString();
}

QString OnlineRankingClient::takenName() const
{
    return m_takenNumber ? m_ranking->playerName(m_takenNumber) : QString();
}

void OnlineRankingClient::setShareState(ShareState state)
{
    m_shareState = state;
    emit shareChanged();
}

void OnlineRankingClient::setSharing(bool on)
{
    if (!m_canShare) on = false;
    if (on == m_sharing) return;
    m_sharing = on;
    if (m_session) m_session->saveString(kShareKey, on ? QStringLiteral("true") : QStringLiteral("false"));
    m_takenNumber = 0;
    m_lastSentTotal = -1;
    if (on) {
        if (m_state != Ready) refresh();  // also gives free numbers for the "taken" flow
        emit shareChanged();
        submitNow();
    } else {
        setShareState(ShareIdle);  // nothing more is sent; the entry expires or can be deleted
    }
}

QUrl OnlineRankingClient::endpointUrl(const QString &endpoint) const
{
    QUrl url(m_submitUrl);
    QString path = url.path();
    while (path.endsWith(QLatin1Char('/'))) path.chop(1);
    url.setPath(path + QLatin1Char('/') + endpoint);
    return url;
}

QNetworkReply *OnlineRankingClient::postJson(const QString &endpoint, const std::string &body)
{
    QNetworkRequest request(endpointUrl(endpoint));
    request.setTransferTimeout(kTimeoutMs);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ZowiDesktop"));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    return m_nam.post(request, QByteArray::fromStdString(body));
}

void OnlineRankingClient::submitNow()
{
    if (!m_canShare || !m_sharing || m_shareReply) return;

    auto &store = m_ranking->store();
    const int number = store.activePlayer();
    if (number == 0) return;

    const auto players = store.players();
    const auto it = std::find_if(players.begin(), players.end(),
                                 [number](const zowi::RankingPlayer &p) { return p.number == number; });
    if (it == players.end()) return;
    if (it->total <= 0) {  // nothing to share yet
        m_sharePosition = 0;
        setShareState(NotQualified);
        return;
    }

    // The secret code only goes with the number it was issued for.
    const bool hadToken = store.onlineNumber() == number;
    const std::string token = hadToken ? store.onlineToken() : std::string();
    m_lastSentTotal = it->total;
    m_takenNumber = 0;
    setShareState(Sending);

    QNetworkReply *reply = postJson(QStringLiteral("submit"),
        zowi::buildSubmitBody(number, it->zowiSays, it->mouths, it->timeline, token));
    m_shareReply = reply;
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, number, hadToken]() { handleSubmitReply(reply, number, hadToken, false); });
}

void OnlineRankingClient::handleSubmitReply(QNetworkReply *reply, int number, bool hadToken, bool retried)
{
    reply->deleteLater();
    m_shareReply.clear();

    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto response = zowi::parseSubmitResponse(http, reply->read(kMaxReplyBytes).toStdString());
    auto &store = m_ranking->store();

    // Sharing was switched off while the request was in flight: just keep the code.
    switch (response.status) {
    case zowi::SubmitStatus::Ok:
        if (!response.token.empty()) store.setOnlineRegistration(number, response.token);
        m_sharePosition = response.position;
        qInfo() << "[RankingOnline] Shared" << m_ranking->playerName(number) << "total" << response.total
                << "position" << response.position;
        setShareState(m_sharing ? Shared : ShareIdle);
        if (m_state == Ready) refresh();
        return;
    case zowi::SubmitStatus::Taken:
        m_takenNumber = number;
        setShareState(Taken);
        return;
    case zowi::SubmitStatus::Unauthorized:
        // The code is no longer valid (the entry expired and someone else took the number):
        // forget it and try once as a new registration.
        if (hadToken && !retried) {
            store.clearOnlineRegistration();
            m_lastSentTotal = -1;
            m_shareState = ShareIdle;
            submitNow();
            return;
        }
        setShareState(ShareError);
        return;
    case zowi::SubmitStatus::NotQualified:
        m_sharePosition = 0;
        setShareState(NotQualified);
        return;
    case zowi::SubmitStatus::RateLimited:
        setShareState(RateLimited);
        return;
    default:
        qInfo() << "[RankingOnline] Submit failed, HTTP" << http;
        m_lastSentTotal = -1;  // try again on the next improvement
        setShareState(ShareError);
        return;
    }
}

void OnlineRankingClient::changeTakenNumber()
{
    auto &store = m_ranking->store();
    const int from = store.activePlayer();
    if (from == 0) return;

    std::vector<int> candidates;
    if (m_worldData.valid) {
        candidates = zowi::freeWorldNumbers(m_worldData);
    } else {
        for (int n = zowi::RankingStore::kMinNumber; n <= zowi::RankingStore::kMaxNumber; ++n) candidates.push_back(n);
    }
    candidates.erase(std::remove_if(candidates.begin(), candidates.end(), [&](int n) {
        return n == m_takenNumber || n == from || store.hasPlayer(n);
    }), candidates.end());
    if (candidates.empty()) { setShareState(ShareError); return; }

    const int to = candidates[QRandomGenerator::global()->bounded(static_cast<int>(candidates.size()))];
    if (!m_ranking->renamePlayer(from, to)) { setShareState(ShareError); return; }

    m_takenNumber = 0;
    m_lastSentTotal = -1;
    submitNow();
}

void OnlineRankingClient::deleteEntry()
{
    if (!m_canShare || m_shareReply) return;
    auto &store = m_ranking->store();
    const int number = store.onlineNumber();
    if (number == 0) {  // nothing registered: just stop sharing
        setSharing(false);
        return;
    }

    setShareState(Sending);
    QNetworkReply *reply = postJson(QStringLiteral("delete"),
                                    zowi::buildDeleteBody(number, store.onlineToken()));
    m_shareReply = reply;
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleDeleteReply(reply); });
}

void OnlineRankingClient::handleDeleteReply(QNetworkReply *reply)
{
    reply->deleteLater();
    m_shareReply.clear();

    const int http = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const auto response = zowi::parseSubmitResponse(http, reply->read(kMaxReplyBytes).toStdString());
    if (response.status == zowi::SubmitStatus::Ok || response.status == zowi::SubmitStatus::Unauthorized) {
        // Deleted, or the code is not valid any more (nothing left to delete): forget it.
        m_ranking->store().clearOnlineRegistration();
        m_sharePosition = 0;
        m_lastSentTotal = -1;
        setSharing(false);
        setShareState(ShareIdle);
        if (m_state == Ready) refresh();
        return;
    }
    setShareState(ShareError);
}
