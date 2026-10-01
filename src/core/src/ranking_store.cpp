#include "zowi/ranking_store.h"

#include <algorithm>
#include <chrono>

#include <nlohmann/json.hpp>

namespace zowi {

using json = nlohmann::json;

namespace {

constexpr RankingGame kAllGames[] = {RankingGame::ZowiSays, RankingGame::Mouths, RankingGame::Timeline};

std::string sanitizeName(const std::string &raw)
{
    const auto first = raw.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "?";
    const auto last = raw.find_last_not_of(" \t\r\n");
    std::string name = raw.substr(first, last - first + 1);
    if (name.size() > RankingStore::kMaxNameLength) name.resize(RankingStore::kMaxNameLength);
    return name;
}

// Position (0-based) where a new score is inserted: after all entries >= score.
size_t insertionIndex(const std::vector<RankingEntry> &entries, int score)
{
    size_t i = 0;
    while (i < entries.size() && entries[i].points >= score) ++i;
    return i;
}

} // namespace

std::string RankingStore::keyFor(RankingGame game)
{
    switch (game) {
    case RankingGame::ZowiSays: return "zowi_says_ranking";
    case RankingGame::Mouths:   return "mouths_ranking";
    case RankingGame::Timeline: return "timeline_ranking";
    }
    return "unknown_ranking";
}

int RankingStore::minScoreToQualify(RankingGame game)
{
    switch (game) {
    case RankingGame::ZowiSays: return 3;
    case RankingGame::Mouths:   return 2;
    case RankingGame::Timeline: return 1;
    }
    return 1;
}

std::vector<RankingEntry> RankingStore::top(RankingGame game) const
{
    std::vector<RankingEntry> out;
    const json parsed = json::parse(m_session.getString(keyFor(game), "[]"), nullptr, false);
    if (!parsed.is_array()) return out;

    for (const auto &item : parsed) {
        if (!item.is_object()) continue;
        RankingEntry e;
        e.points = item.value("points", 0);
        e.playerName = item.value("playerName", std::string());
        e.timestamp = item.value("timestamp", int64_t{0});
        out.push_back(std::move(e));
    }
    std::stable_sort(out.begin(), out.end(),
                     [](const RankingEntry &a, const RankingEntry &b) { return a.points > b.points; });
    if (out.size() > static_cast<size_t>(kMaxEntries)) out.resize(kMaxEntries);
    return out;
}

bool RankingStore::qualifies(RankingGame game, int score) const
{
    if (score < minScoreToQualify(game)) return false;
    const auto entries = top(game);
    if (entries.size() < static_cast<size_t>(kMaxEntries)) return true;
    return score > entries.back().points;
}

int RankingStore::submit(RankingGame game, const std::string &playerName, int score, int64_t timestamp)
{
    if (!qualifies(game, score)) return 0;

    if (timestamp == 0) {
        timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch()).count();
    }

    auto entries = top(game);
    const size_t index = insertionIndex(entries, score);
    entries.insert(entries.begin() + static_cast<std::ptrdiff_t>(index),
                   RankingEntry{score, sanitizeName(playerName), timestamp});
    if (entries.size() > static_cast<size_t>(kMaxEntries)) entries.resize(kMaxEntries);

    save(game, entries);
    return static_cast<int>(index) + 1;
}

int RankingStore::best(RankingGame game) const
{
    const auto entries = top(game);
    return entries.empty() ? 0 : entries.front().points;
}

void RankingStore::clear(RankingGame game)
{
    m_session.removeKey(keyFor(game));
}

void RankingStore::clearAll()
{
    for (const auto game : kAllGames) clear(game);
}

void RankingStore::save(RankingGame game, const std::vector<RankingEntry> &entries)
{
    json arr = json::array();
    for (const auto &e : entries) {
        arr.push_back({{"points", e.points}, {"playerName", e.playerName}, {"timestamp", e.timestamp}});
    }
    m_session.setString(keyFor(game), arr.dump());
}

} // namespace zowi
