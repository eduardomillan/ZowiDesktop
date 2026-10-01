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

// Reference score per game used to normalise points: a score equal to the
// reference is worth 100 points. Scores of different games are not comparable
// raw (Zowi Says = length-1, Mouths = level-1, Timeline = weighted complexity).
struct RankingScoreConfig {
    int referenceZowiSays = 12;
    int referenceMouths = 8;
    int referenceTimeline = 60;

    int reference(RankingGame game) const;
};

// One local player. The number (100-999) is both the nickname and the id:
// it is shown as "Player-123". Free text never enters the ranking.
struct RankingPlayer {
    int number = 0;
    int zowiSays = 0;   // best raw score per game
    int mouths = 0;
    int timeline = 0;
    int64_t updated = 0;  // seconds since epoch of the last improvement
    int total = 0;        // normalised sum of the three bests
    int position = 0;     // 1-based, 0 when unknown
};

struct RankingResult {
    bool improved = false;  // the raw score beat the player's best for that game
    int number = 0;         // player the score was recorded for (0 = none)
    int total = 0;
    int position = 0;
};

// Single global ranking: every game adds its best score (normalised) to the
// player's total. Stored in its own file (<config dir>/ZowiRanking.json),
// separate from the session file (ZowiApp.json), so wiping the session never
// touches rankings. The app can add and read entries but never delete them:
// removal is an admin task (zowi_cli ranking).
class RankingStore {
public:
    static constexpr int kMinNumber = 100;
    static constexpr int kMaxNumber = 999;
    static constexpr int kDefaultTop = 10;

    // `configDir` optionally pins the directory (platform default when empty),
    // like SessionStore. Hosts pass their app data dir; tests pass a temp dir.
    explicit RankingStore(const std::string &configDir = "", RankingScoreConfig config = {})
        : m_store("ZowiDesktop", "ZowiRanking", configDir), m_config(config) {
        // The file can hold the online ownership token: owner-only permissions.
        m_store.restrictToOwner();
    }

    // 100-999: three digits, never starting with 0.
    static bool isValidNumber(int number);
    // "Player-123" (the prefix is a fixed literal in every language).
    static std::string displayName(int number);
    // Parses "Player-123" or "123"; 0 when invalid.
    static int parseNumber(const std::string &text);

    // Normalised points for a raw best score in a game.
    int pointsFor(RankingGame game, int rawBest) const;

    // ── players ─────────────────────────────────────────────────────────
    std::vector<RankingPlayer> players() const;  // ranked: total desc, earlier first on ties
    bool hasPlayer(int number) const;
    // A random valid number that is not taken (0 when all 900 are used).
    int suggestFreeNumber() const;
    // False when the number is invalid or already taken. The first player
    // created becomes the active one.
    bool createPlayer(int number);
    int activePlayer() const;  // 0 when there is none
    bool setActivePlayer(int number);
    // Creates and activates a random player when there is none; returns it.
    int ensureActivePlayer();
    // Changes a player's number keeping their scores (online name clash). If
    // `from` was registered online, that registration (and its token) is dropped.
    bool renamePlayer(int from, int to);

    // ── online registration (world ranking) ─────────────────────────────
    // The secret returned by the server when a player is registered online.
    // Never published; the file is kept owner-only (0600).
    int onlineNumber() const;           // 0 when not registered online
    std::string onlineToken() const;    // "" when not registered online
    void setOnlineRegistration(int number, const std::string &token);
    void clearOnlineRegistration();

    // ── scoring ─────────────────────────────────────────────────────────
    // Records a raw score for the active player (created on demand). Only a
    // score above the stored best changes anything. Timestamp 0 = now.
    RankingResult recordScore(RankingGame game, int rawScore, int64_t timestamp = 0);
    int total(int number) const;
    int position(int number) const;  // 0 when the player does not exist

    // Best `limit` players (default 10).
    std::vector<RankingPlayer> top(int limit = kDefaultTop) const;

    // ── admin only (CLI) ────────────────────────────────────────────────
    bool removePlayer(int number);
    void clear();  // every player and the active one, including legacy keys

private:
    SessionStore m_store;  // generic JSON key-value file, used only for rankings
    RankingScoreConfig m_config;

    std::vector<RankingPlayer> load() const;
    void save(const std::vector<RankingPlayer> &players);
    void rank(std::vector<RankingPlayer> &players) const;
};

} // namespace zowi
