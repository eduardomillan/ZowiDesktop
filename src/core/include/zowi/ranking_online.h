#pragma once

#include <string>
#include <vector>

namespace zowi {

// Pure helpers for the optional world ranking: reading and validating the
// public JSON, building the request bodies and interpreting the server's
// replies. No network code here (the GUI owns the transport), so everything is
// testable. The server is the authority; the client only uses this data to
// decide what to show and to suggest free numbers.

constexpr int kWorldMaxEntries = 100;

struct WorldEntry {
    int number = 0;     // 100-999 ("Player-NNN")
    int total = 0;      // normalised points
    int zowiSays = 0;   // normalised points per game
    int mouths = 0;
    int timeline = 0;
};

struct WorldRanking {
    bool valid = false;           // false when the JSON could not be read at all
    std::string generated;        // timestamp text from the server (informational)
    std::vector<WorldEntry> players;  // best first, at most kWorldMaxEntries
};

// Parses the public ranking JSON. Malformed entries (number outside 100-999,
// negative values, duplicates, wrong types) are dropped; more than 100 rows
// are cut; rows are sorted by total (desc) then number. `valid` is false when
// the document is not an object with a "players" array.
WorldRanking parseWorldRanking(const std::string &json);

// Total a player must exceed to enter: 0 while there are fewer than 100 rows,
// otherwise the total of the 100th row.
int worldCutoff(const WorldRanking &world);
bool beatsWorldCutoff(const WorldRanking &world, int total);

bool worldHasNumber(const WorldRanking &world, int number);
// Numbers 100-999 that are not in the list.
std::vector<int> freeWorldNumbers(const WorldRanking &world);
// A random free number, or 0 when none is left.
int suggestWorldNumber(const WorldRanking &world);
// 1-based position of `number` in the list, 0 when absent.
int worldPosition(const WorldRanking &world, int number);

// Request bodies (JSON). The client sends raw bests; the server computes the total.
std::string buildSubmitBody(int number, int zowiSaysBest, int mouthsBest, int timelineBest,
                            const std::string &token = "");
std::string buildDeleteBody(int number, const std::string &token);

enum class SubmitStatus {
    Ok,            // accepted (registered or updated)
    Taken,         // the number belongs to someone else
    NotQualified,  // the total does not enter the top 100 (or did not improve)
    Unauthorized,  // missing/wrong token
    RateLimited,   // too many requests today
    Invalid,       // the server rejected the data
    ServerError,   // 5xx or an unreadable reply
    Network        // no HTTP reply at all
};

struct SubmitResponse {
    SubmitStatus status = SubmitStatus::ServerError;
    std::string token;  // only present when a new registration is created
    int total = 0;
    int position = 0;
};

// Interprets an HTTP status (0 = no reply) and body from /submit or /delete.
SubmitResponse parseSubmitResponse(int httpStatus, const std::string &body);

} // namespace zowi
