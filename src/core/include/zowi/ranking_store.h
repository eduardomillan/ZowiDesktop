#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <zowi/session_store.h>

namespace zowi {

enum class RankingGame {
    ZowiSays,
    Mouths,
    Timeline
};

struct RankingEntry {
    int points = 0;
    std::string playerName;
    int64_t timestamp = 0;  // seconds since epoch
};

// Local top-N ranking per game. Rankings live in their own file
// (<config dir>/ZowiRanking.json), separate from the session file
// (ZowiApp.json), so wiping or resetting the session never touches them.
// Each game's list is a JSON array under "<game>" (same shape as the Android
// original: points, playerName, timestamp; sorted descending, capped at
// kMaxEntries).
class RankingStore {
public:
    static constexpr int kMaxEntries = 10;
    static constexpr size_t kMaxNameLength = 12;

    // `configDir` optionally pins the directory (platform default when empty),
    // like SessionStore. Hosts pass their app data dir; tests pass a temp dir.
    explicit RankingStore(const std::string &configDir = "")
        : m_store("ZowiDesktop", "ZowiRanking", configDir) {}

    // Key of a game's ranking list inside the ranking file.
    static std::string keyFor(RankingGame game);
    // Minimum score that may enter the ranking (Android: Says size>3, Mouths level>2).
    static int minScoreToQualify(RankingGame game);

    std::vector<RankingEntry> top(RankingGame game) const;

    // True when `score` meets the game's minimum and would land in the top N.
    bool qualifies(RankingGame game, int score) const;

    // Inserts the entry (name trimmed/truncated, "?" if blank). Returns the
    // 1-based position, or 0 when the score did not qualify. Ties rank after
    // existing equal scores.
    int submit(RankingGame game, const std::string &playerName, int score, int64_t timestamp = 0);

    // Highest stored score, or 0 when the ranking is empty.
    int best(RankingGame game) const;

    void clear(RankingGame game);
    void clearAll();

private:
    SessionStore m_store;  // generic JSON key-value file, used only for rankings

    void save(RankingGame game, const std::vector<RankingEntry> &entries);
};

} // namespace zowi
