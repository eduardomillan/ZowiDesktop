#include "zowi/ranking_store.h"

#include <algorithm>
#include <chrono>
#include <random>

#include <nlohmann/json.hpp>

namespace zowi {

using json = nlohmann::json;

namespace {

constexpr const char *kPlayersKey = "players";
constexpr const char *kActiveKey = "active_player";
constexpr const char *kNamePrefix = "Player-";

int64_t nowSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch()).count();
}

int &bestFor(RankingPlayer &p, RankingGame game)
{
    switch (game) {
    case RankingGame::ZowiSays: return p.zowiSays;
    case RankingGame::Mouths:   return p.mouths;
    case RankingGame::Timeline: return p.timeline;
    }
    return p.zowiSays;
}

} // namespace

int RankingScoreConfig::reference(RankingGame game) const
{
    switch (game) {
    case RankingGame::ZowiSays: return referenceZowiSays;
    case RankingGame::Mouths:   return referenceMouths;
    case RankingGame::Timeline: return referenceTimeline;
    }
    return 1;
}

bool RankingStore::isValidNumber(int number)
{
    return number >= kMinNumber && number <= kMaxNumber;
}

std::string RankingStore::displayName(int number)
{
    return std::string(kNamePrefix) + std::to_string(number);
}

int RankingStore::parseNumber(const std::string &text)
{
    std::string digits = text;
    const std::string prefix = kNamePrefix;
    if (digits.rfind(prefix, 0) == 0) digits = digits.substr(prefix.size());
    if (digits.size() != 3) return 0;
    for (const char c : digits) {
        if (c < '0' || c > '9') return 0;
    }
    const int number = std::stoi(digits);
    return isValidNumber(number) ? number : 0;
}

int RankingStore::pointsFor(RankingGame game, int rawBest) const
{
    if (rawBest <= 0) return 0;
    const int reference = std::max(1, m_config.reference(game));
    return (100 * rawBest + reference / 2) / reference;
}

void RankingStore::rank(std::vector<RankingPlayer> &players) const
{
    for (auto &p : players) {
        p.total = pointsFor(RankingGame::ZowiSays, p.zowiSays)
                + pointsFor(RankingGame::Mouths, p.mouths)
                + pointsFor(RankingGame::Timeline, p.timeline);
    }
    std::stable_sort(players.begin(), players.end(), [](const RankingPlayer &a, const RankingPlayer &b) {
        if (a.total != b.total) return a.total > b.total;
        if (a.updated != b.updated) return a.updated < b.updated;  // reached it first
        return a.number < b.number;
    });
    int position = 1;
    for (auto &p : players) p.position = position++;
}

std::vector<RankingPlayer> RankingStore::load() const
{
    std::vector<RankingPlayer> out;
    const json parsed = json::parse(m_store.getString(kPlayersKey, "[]"), nullptr, false);
    if (!parsed.is_array()) return out;

    for (const auto &item : parsed) {
        if (!item.is_object()) continue;
        RankingPlayer p;
        p.number = item.value("id", 0);
        if (!isValidNumber(p.number)) continue;
        if (std::any_of(out.begin(), out.end(), [&](const RankingPlayer &o) { return o.number == p.number; })) continue;
        p.zowiSays = std::max(0, item.value("zowi_says", 0));
        p.mouths = std::max(0, item.value("mouths", 0));
        p.timeline = std::max(0, item.value("timeline", 0));
        p.updated = item.value("updated", int64_t{0});
        out.push_back(p);
    }
    rank(out);
    return out;
}

void RankingStore::save(const std::vector<RankingPlayer> &players)
{
    json arr = json::array();
    for (const auto &p : players) {
        arr.push_back({{"id", p.number},
                       {"zowi_says", p.zowiSays},
                       {"mouths", p.mouths},
                       {"timeline", p.timeline},
                       {"updated", p.updated}});
    }
    m_store.setString(kPlayersKey, arr.dump());
}

std::vector<RankingPlayer> RankingStore::players() const
{
    return load();
}

bool RankingStore::hasPlayer(int number) const
{
    const auto all = load();
    return std::any_of(all.begin(), all.end(), [&](const RankingPlayer &p) { return p.number == number; });
}

int RankingStore::suggestFreeNumber() const
{
    const auto all = load();
    std::vector<bool> taken(kMaxNumber + 1, false);
    for (const auto &p : all) taken[p.number] = true;

    std::vector<int> free;
    for (int n = kMinNumber; n <= kMaxNumber; ++n) {
        if (!taken[n]) free.push_back(n);
    }
    if (free.empty()) return 0;

    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> pick(0, free.size() - 1);
    return free[pick(rng)];
}

bool RankingStore::createPlayer(int number)
{
    if (!isValidNumber(number) || hasPlayer(number)) return false;

    auto all = load();
    RankingPlayer p;
    p.number = number;
    p.updated = nowSeconds();
    all.push_back(p);
    save(all);

    if (activePlayer() == 0) m_store.setInt(kActiveKey, number);
    return true;
}

int RankingStore::activePlayer() const
{
    const int number = m_store.getInt(kActiveKey, 0);
    return hasPlayer(number) ? number : 0;
}

bool RankingStore::setActivePlayer(int number)
{
    if (!hasPlayer(number)) return false;
    m_store.setInt(kActiveKey, number);
    return true;
}

int RankingStore::ensureActivePlayer()
{
    if (const int active = activePlayer()) return active;

    const auto all = load();
    if (!all.empty()) {  // the active one was removed: fall back to the best player
        m_store.setInt(kActiveKey, all.front().number);
        return all.front().number;
    }
    const int number = suggestFreeNumber();
    if (number != 0 && createPlayer(number)) return number;
    return 0;
}

bool RankingStore::renamePlayer(int from, int to)
{
    if (from == to || !isValidNumber(to) || hasPlayer(to)) return false;

    auto all = load();
    auto it = std::find_if(all.begin(), all.end(), [&](const RankingPlayer &p) { return p.number == from; });
    if (it == all.end()) return false;

    const bool wasActive = activePlayer() == from;
    it->number = to;
    save(all);
    if (wasActive) m_store.setInt(kActiveKey, to);
    return true;
}

RankingResult RankingStore::recordScore(RankingGame game, int rawScore, int64_t timestamp)
{
    RankingResult result;
    const int number = ensureActivePlayer();
    if (number == 0) return result;
    result.number = number;

    auto all = load();
    auto it = std::find_if(all.begin(), all.end(), [&](const RankingPlayer &p) { return p.number == number; });
    if (it == all.end()) return result;

    int &best = bestFor(*it, game);
    if (rawScore > best) {
        best = rawScore;
        it->updated = timestamp != 0 ? timestamp : nowSeconds();
        result.improved = true;
        save(all);
        rank(all);
        it = std::find_if(all.begin(), all.end(), [&](const RankingPlayer &p) { return p.number == number; });
    }
    result.total = it->total;
    result.position = it->position;
    return result;
}

int RankingStore::total(int number) const
{
    for (const auto &p : load()) {
        if (p.number == number) return p.total;
    }
    return 0;
}

int RankingStore::position(int number) const
{
    for (const auto &p : load()) {
        if (p.number == number) return p.position;
    }
    return 0;
}

std::vector<RankingPlayer> RankingStore::top(int limit) const
{
    auto all = load();
    if (limit >= 0 && all.size() > static_cast<size_t>(limit)) all.resize(limit);
    return all;
}

bool RankingStore::removePlayer(int number)
{
    auto all = load();
    const auto it = std::remove_if(all.begin(), all.end(), [&](const RankingPlayer &p) { return p.number == number; });
    if (it == all.end()) return false;
    all.erase(it, all.end());
    save(all);
    return true;
}

void RankingStore::clear()
{
    for (const auto &key : m_store.keys()) m_store.removeKey(key);
}

} // namespace zowi
