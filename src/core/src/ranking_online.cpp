#include "zowi/ranking_online.h"

#include <algorithm>
#include <random>
#include <set>

#include <nlohmann/json.hpp>

namespace zowi {

using json = nlohmann::json;

namespace {

constexpr int kMinNumber = 100;
constexpr int kMaxNumber = 999;

bool readInt(const json &obj, const char *key, int &out, bool required)
{
    if (!obj.contains(key)) return !required;
    const auto &v = obj[key];
    if (!v.is_number_integer()) return false;
    out = v.get<int>();
    return true;
}

} // namespace

WorldRanking parseWorldRanking(const std::string &text)
{
    WorldRanking world;
    const json doc = json::parse(text, nullptr, false);
    if (!doc.is_object() || !doc.contains("players") || !doc["players"].is_array()) return world;

    world.valid = true;
    if (doc.contains("generated") && doc["generated"].is_string())
        world.generated = doc["generated"].get<std::string>();

    std::set<int> seen;
    for (const auto &item : doc["players"]) {
        if (!item.is_object()) continue;
        WorldEntry e;
        if (!readInt(item, "number", e.number, true) || e.number < kMinNumber || e.number > kMaxNumber) continue;
        if (!readInt(item, "total", e.total, true)) continue;
        if (!readInt(item, "zowi_says", e.zowiSays, false)) continue;
        if (!readInt(item, "mouths", e.mouths, false)) continue;
        if (!readInt(item, "timeline", e.timeline, false)) continue;
        if (e.total < 0 || e.zowiSays < 0 || e.mouths < 0 || e.timeline < 0) continue;
        if (!seen.insert(e.number).second) continue;
        world.players.push_back(e);
    }

    std::stable_sort(world.players.begin(), world.players.end(), [](const WorldEntry &a, const WorldEntry &b) {
        if (a.total != b.total) return a.total > b.total;
        return a.number < b.number;
    });
    if (world.players.size() > static_cast<size_t>(kWorldMaxEntries)) world.players.resize(kWorldMaxEntries);
    return world;
}

int worldCutoff(const WorldRanking &world)
{
    if (world.players.size() < static_cast<size_t>(kWorldMaxEntries)) return 0;
    return world.players.back().total;
}

bool beatsWorldCutoff(const WorldRanking &world, int total)
{
    return total > 0 && total > worldCutoff(world);
}

bool worldHasNumber(const WorldRanking &world, int number)
{
    return std::any_of(world.players.begin(), world.players.end(),
                       [&](const WorldEntry &e) { return e.number == number; });
}

std::vector<int> freeWorldNumbers(const WorldRanking &world)
{
    std::set<int> taken;
    for (const auto &e : world.players) taken.insert(e.number);
    std::vector<int> free;
    for (int n = kMinNumber; n <= kMaxNumber; ++n) {
        if (!taken.count(n)) free.push_back(n);
    }
    return free;
}

int suggestWorldNumber(const WorldRanking &world)
{
    const auto free = freeWorldNumbers(world);
    if (free.empty()) return 0;
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<size_t> pick(0, free.size() - 1);
    return free[pick(rng)];
}

int worldPosition(const WorldRanking &world, int number)
{
    for (size_t i = 0; i < world.players.size(); ++i) {
        if (world.players[i].number == number) return static_cast<int>(i) + 1;
    }
    return 0;
}

std::string buildSubmitBody(int number, int zowiSaysBest, int mouthsBest, int timelineBest,
                            const std::string &token)
{
    json body = {{"number", number},
                 {"zowi_says", std::max(0, zowiSaysBest)},
                 {"mouths", std::max(0, mouthsBest)},
                 {"timeline", std::max(0, timelineBest)}};
    if (!token.empty()) body["token"] = token;
    return body.dump();
}

std::string buildDeleteBody(int number, const std::string &token)
{
    return json{{"number", number}, {"token", token}}.dump();
}

SubmitResponse parseSubmitResponse(int httpStatus, const std::string &body)
{
    SubmitResponse r;
    if (httpStatus == 0) { r.status = SubmitStatus::Network; return r; }

    const json doc = json::parse(body, nullptr, false);

    if (httpStatus >= 200 && httpStatus < 300) {
        if (!doc.is_object()) { r.status = SubmitStatus::ServerError; return r; }
        r.status = SubmitStatus::Ok;
        if (doc.contains("token") && doc["token"].is_string()) r.token = doc["token"].get<std::string>();
        if (doc.contains("total") && doc["total"].is_number_integer()) r.total = doc["total"].get<int>();
        if (doc.contains("position") && doc["position"].is_number_integer()) r.position = doc["position"].get<int>();
        return r;
    }
    switch (httpStatus) {
    case 409: r.status = SubmitStatus::Taken; break;
    case 422: r.status = SubmitStatus::NotQualified; break;
    case 401:
    case 403: r.status = SubmitStatus::Unauthorized; break;
    case 429: r.status = SubmitStatus::RateLimited; break;
    case 400: r.status = SubmitStatus::Invalid; break;
    default:  r.status = SubmitStatus::ServerError; break;
    }
    return r;
}

} // namespace zowi
