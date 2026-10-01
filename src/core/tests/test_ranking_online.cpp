#include <zowi/ranking_online.h>

#include <cassert>
#include <iostream>
#include <set>
#include <string>

#include <nlohmann/json.hpp>

using namespace zowi;

static std::string makeJson(int count, int startNumber = 100) {
    nlohmann::json players = nlohmann::json::array();
    for (int i = 0; i < count; ++i) {
        players.push_back({{"number", startNumber + i}, {"total", 1000 - i}, {"zowi_says", 10}, {"mouths", 5}, {"timeline", 0}});
    }
    return nlohmann::json{{"generated", "2026-10-02T12:00:00Z"}, {"cutoff", 0}, {"players", players}}.dump();
}

void test_parse_valid_and_order() {
    std::cout << "test_parse_valid_and_order: " << std::flush;
    const auto w = parseWorldRanking(R"({"generated":"t","players":[
        {"number":300,"total":50,"zowi_says":50},
        {"number":200,"total":150,"zowi_says":100,"mouths":50,"timeline":0},
        {"number":150,"total":50}]})");
    assert(w.valid && w.generated == "t");
    assert(w.players.size() == 3);
    assert(w.players[0].number == 200 && w.players[0].total == 150);
    assert(w.players[1].number == 150 && w.players[2].number == 300);  // tie: lower number first
    assert(worldPosition(w, 300) == 3 && worldPosition(w, 999) == 0);
    std::cout << "OK" << std::endl;
}

void test_parse_drops_bad_entries() {
    std::cout << "test_parse_drops_bad_entries: " << std::flush;
    const auto w = parseWorldRanking(R"({"players":[
        {"number":99,"total":10},{"number":1000,"total":10},{"number":"123","total":10},
        {"number":123,"total":-5},{"number":124,"total":"x"},{"number":125,"total":10,"mouths":-1},
        {"number":200,"total":10},{"number":200,"total":99},"junk",7,
        {"number":201,"total":20}]})");
    assert(w.valid);
    assert(w.players.size() == 2);          // only 201 and the first 200
    assert(w.players[0].number == 201);
    assert(w.players[1].number == 200 && w.players[1].total == 10);
    std::cout << "OK" << std::endl;
}

void test_parse_invalid_documents() {
    std::cout << "test_parse_invalid_documents: " << std::flush;
    assert(!parseWorldRanking("not json").valid);
    assert(!parseWorldRanking("[1,2]").valid);
    assert(!parseWorldRanking(R"({"players":"nope"})").valid);
    assert(!parseWorldRanking("").valid);
    assert(parseWorldRanking(R"({"players":[]})").valid);
    std::cout << "OK" << std::endl;
}

void test_cap_and_cutoff() {
    std::cout << "test_cap_and_cutoff: " << std::flush;
    auto few = parseWorldRanking(makeJson(10));
    assert(worldCutoff(few) == 0);
    assert(beatsWorldCutoff(few, 1) && !beatsWorldCutoff(few, 0));   // room left: any total > 0

    auto full = parseWorldRanking(makeJson(150));   // more than 100 rows: cut to 100
    assert(full.players.size() == 100);
    assert(worldCutoff(full) == full.players.back().total);
    assert(!beatsWorldCutoff(full, full.players.back().total));      // must be strictly higher
    assert(beatsWorldCutoff(full, full.players.back().total + 1));
    std::cout << "OK" << std::endl;
}

void test_free_numbers() {
    std::cout << "test_free_numbers: " << std::flush;
    auto w = parseWorldRanking(makeJson(100, 100));     // numbers 100-199 taken
    const auto free = freeWorldNumbers(w);
    assert(free.size() == 900 - 100);
    assert(free.front() == 200 && free.back() == 999);
    for (int i = 0; i < 50; ++i) {
        const int n = suggestWorldNumber(w);
        assert(n >= 200 && n <= 999 && !worldHasNumber(w, n));
    }
    // Everything taken (cannot happen with 100 rows, but the helper must cope).
    WorldRanking full;
    full.valid = true;
    for (int n = 100; n <= 999; ++n) full.players.push_back({n, 1, 0, 0, 0});
    assert(suggestWorldNumber(full) == 0);
    std::cout << "OK" << std::endl;
}

void test_bodies() {
    std::cout << "test_bodies: " << std::flush;
    auto b = nlohmann::json::parse(buildSubmitBody(123, 6, 4, 30));
    assert(b["number"] == 123 && b["zowi_says"] == 6 && b["mouths"] == 4 && b["timeline"] == 30);
    assert(!b.contains("token"));
    auto withToken = nlohmann::json::parse(buildSubmitBody(123, -3, 4, 30, "abc"));
    assert(withToken["token"] == "abc" && withToken["zowi_says"] == 0);   // negatives clamp to 0
    auto d = nlohmann::json::parse(buildDeleteBody(123, "tok"));
    assert(d["number"] == 123 && d["token"] == "tok");
    std::cout << "OK" << std::endl;
}

void test_responses() {
    std::cout << "test_responses: " << std::flush;
    auto ok = parseSubmitResponse(200, R"({"ok":true,"token":"secret","total":150,"position":3})");
    assert(ok.status == SubmitStatus::Ok && ok.token == "secret" && ok.total == 150 && ok.position == 3);
    auto update = parseSubmitResponse(200, R"({"ok":true,"total":160,"position":2})");
    assert(update.status == SubmitStatus::Ok && update.token.empty());
    assert(parseSubmitResponse(200, "garbage").status == SubmitStatus::ServerError);
    assert(parseSubmitResponse(409, "{}").status == SubmitStatus::Taken);
    assert(parseSubmitResponse(422, "{}").status == SubmitStatus::NotQualified);
    assert(parseSubmitResponse(401, "").status == SubmitStatus::Unauthorized);
    assert(parseSubmitResponse(403, "").status == SubmitStatus::Unauthorized);
    assert(parseSubmitResponse(429, "").status == SubmitStatus::RateLimited);
    assert(parseSubmitResponse(400, "").status == SubmitStatus::Invalid);
    assert(parseSubmitResponse(500, "").status == SubmitStatus::ServerError);
    assert(parseSubmitResponse(503, "x").status == SubmitStatus::ServerError);
    assert(parseSubmitResponse(0, "").status == SubmitStatus::Network);
    std::cout << "OK" << std::endl;
}

int main() {
    test_parse_valid_and_order();
    test_parse_drops_bad_entries();
    test_parse_invalid_documents();
    test_cap_and_cutoff();
    test_free_numbers();
    test_bodies();
    test_responses();
    std::cout << "All ranking_online tests passed." << std::endl;
    return 0;
}
