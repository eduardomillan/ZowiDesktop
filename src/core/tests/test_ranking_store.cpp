#include <zowi/ranking_store.h>

#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>

namespace fs = std::filesystem;
using zowi::RankingGame;
using zowi::RankingStore;

namespace {

struct TempStore {
    fs::path dir;
    RankingStore ranking;

    static fs::path makeDir(const char *name) {
        fs::path d = fs::temp_directory_path() / (std::string("zowi_ranking_test_") + name);
        fs::remove_all(d);
        fs::create_directories(d);
        return d;
    }
    explicit TempStore(const char *name) : dir(makeDir(name)), ranking(dir.string()) {}
    ~TempStore() { fs::remove_all(dir); }
};

} // namespace

void test_numbers_and_names() {
    std::cout << "test_numbers_and_names: " << std::flush;
    assert(!RankingStore::isValidNumber(99));
    assert(RankingStore::isValidNumber(100));
    assert(RankingStore::isValidNumber(999));
    assert(!RankingStore::isValidNumber(1000));
    assert(!RankingStore::isValidNumber(0));
    assert(RankingStore::displayName(123) == "Player-123");
    assert(RankingStore::parseNumber("Player-123") == 123);
    assert(RankingStore::parseNumber("456") == 456);
    assert(RankingStore::parseNumber("099") == 0);   // leading zero
    assert(RankingStore::parseNumber("Player-12") == 0);
    assert(RankingStore::parseNumber("Player-1234") == 0);
    assert(RankingStore::parseNumber("abc") == 0);
    std::cout << "OK" << std::endl;
}

void test_create_players() {
    std::cout << "test_create_players: " << std::flush;
    TempStore t("create");
    assert(t.ranking.players().empty());
    assert(t.ranking.activePlayer() == 0);
    assert(!t.ranking.createPlayer(50));    // invalid
    assert(!t.ranking.createPlayer(0));
    assert(t.ranking.createPlayer(123));
    assert(t.ranking.activePlayer() == 123);  // first player becomes active
    assert(!t.ranking.createPlayer(123));   // taken
    assert(t.ranking.createPlayer(456));
    assert(t.ranking.activePlayer() == 123);  // creating another does not switch
    assert(t.ranking.setActivePlayer(456));
    assert(t.ranking.activePlayer() == 456);
    assert(!t.ranking.setActivePlayer(777));
    assert(t.ranking.players().size() == 2);
    std::cout << "OK" << std::endl;
}

void test_suggest_free_number() {
    std::cout << "test_suggest_free_number: " << std::flush;
    TempStore t("suggest");
    for (int i = 0; i < 50; ++i) {
        const int n = t.ranking.suggestFreeNumber();
        assert(RankingStore::isValidNumber(n));
        assert(!t.ranking.hasPlayer(n));
    }
    // Fill all but one number: the suggestion must be the only free one.
    for (int n = RankingStore::kMinNumber; n <= RankingStore::kMaxNumber; ++n) {
        if (n != 500) assert(t.ranking.createPlayer(n));
    }
    assert(t.ranking.suggestFreeNumber() == 500);
    assert(t.ranking.createPlayer(500));
    assert(t.ranking.suggestFreeNumber() == 0);   // full
    std::cout << "OK" << std::endl;
}

void test_scoring_and_normalisation() {
    std::cout << "test_scoring_and_normalisation: " << std::flush;
    TempStore t("scoring");
    // 12/12 Says = 100, 8/8 Mouths = 100, 60/60 Timeline = 100
    assert(t.ranking.pointsFor(RankingGame::ZowiSays, 12) == 100);
    assert(t.ranking.pointsFor(RankingGame::Mouths, 8) == 100);
    assert(t.ranking.pointsFor(RankingGame::Timeline, 60) == 100);
    assert(t.ranking.pointsFor(RankingGame::Timeline, 30) == 50);
    assert(t.ranking.pointsFor(RankingGame::Mouths, 0) == 0);
    assert(t.ranking.pointsFor(RankingGame::ZowiSays, 24) == 200);  // no cap

    auto r = t.ranking.recordScore(RankingGame::ZowiSays, 6, 10);  // creates the player
    assert(r.improved && r.number != 0 && r.total == 50 && r.position == 1);
    assert(t.ranking.activePlayer() == r.number);

    r = t.ranking.recordScore(RankingGame::ZowiSays, 4, 11);   // lower: ignored
    assert(!r.improved && r.total == 50);
    r = t.ranking.recordScore(RankingGame::ZowiSays, 6, 12);   // equal: not an improvement
    assert(!r.improved && r.total == 50);
    r = t.ranking.recordScore(RankingGame::ZowiSays, 12, 13);
    assert(r.improved && r.total == 100);
    r = t.ranking.recordScore(RankingGame::Mouths, 4, 14);     // other game adds up
    assert(r.improved && r.total == 150);
    assert(t.ranking.total(r.number) == 150);
    std::cout << "OK" << std::endl;
}

void test_ranking_order_and_ties() {
    std::cout << "test_ranking_order_and_ties: " << std::flush;
    TempStore t("order");
    assert(t.ranking.createPlayer(200));
    assert(t.ranking.createPlayer(300));
    assert(t.ranking.createPlayer(400));

    t.ranking.setActivePlayer(200);
    t.ranking.recordScore(RankingGame::Mouths, 4, 100);        // 50
    t.ranking.setActivePlayer(300);
    t.ranking.recordScore(RankingGame::Mouths, 8, 200);        // 100
    t.ranking.setActivePlayer(400);
    auto r = t.ranking.recordScore(RankingGame::Mouths, 4, 50); // 50, reached before 200
    assert(r.position == 2);                                    // ties: earlier first

    const auto top = t.ranking.top();
    assert(top.size() == 3);
    assert(top[0].number == 300 && top[0].position == 1);
    assert(top[1].number == 400);
    assert(top[2].number == 200);
    assert(t.ranking.position(200) == 3);
    assert(t.ranking.position(999) == 0);
    assert(t.ranking.top(2).size() == 2);
    std::cout << "OK" << std::endl;
}

void test_rename_and_remove() {
    std::cout << "test_rename_and_remove: " << std::flush;
    TempStore t("rename");
    t.ranking.createPlayer(200);
    t.ranking.recordScore(RankingGame::Mouths, 8, 10);
    t.ranking.createPlayer(300);
    assert(!t.ranking.renamePlayer(200, 300));  // taken
    assert(!t.ranking.renamePlayer(200, 50));   // invalid
    assert(!t.ranking.renamePlayer(555, 556));  // unknown
    assert(t.ranking.renamePlayer(200, 250));
    assert(!t.ranking.hasPlayer(200));
    assert(t.ranking.total(250) == 100);        // scores kept
    assert(t.ranking.activePlayer() == 250);    // active follows the rename

    assert(t.ranking.removePlayer(250));
    assert(!t.ranking.removePlayer(250));
    assert(t.ranking.activePlayer() == 0);
    assert(t.ranking.ensureActivePlayer() == 300);  // falls back to an existing player
    std::cout << "OK" << std::endl;
}

void test_persistence_and_clear() {
    std::cout << "test_persistence_and_clear: " << std::flush;
    fs::path dir = TempStore::makeDir("persist");
    {
        RankingStore r(dir.string());
        r.createPlayer(321);
        r.recordScore(RankingGame::Timeline, 30, 5);
    }
    {
        RankingStore r(dir.string());
        assert(r.activePlayer() == 321);
        assert(r.total(321) == 50);
        r.clear();
        assert(r.players().empty());
        assert(r.activePlayer() == 0);
    }
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_corrupt_and_legacy_data() {
    std::cout << "test_corrupt_and_legacy_data: " << std::flush;
    fs::path dir = TempStore::makeDir("corrupt");
    {
        zowi::SessionStore raw("ZowiDesktop", "ZowiRanking", dir.string());
        raw.setString("players", "not json");
        raw.setString("mouths", "[{\"playerName\":\"Old\",\"points\":5}]");  // legacy per-game list
    }
    RankingStore r(dir.string());
    assert(r.players().empty());
    assert(r.recordScore(RankingGame::Mouths, 4, 1).improved);
    r.clear();  // admin clear also wipes legacy keys
    zowi::SessionStore raw("ZowiDesktop", "ZowiRanking", dir.string());
    assert(raw.keys().empty());
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_separate_file_from_session() {
    std::cout << "test_separate_file_from_session: " << std::flush;
    fs::path dir = TempStore::makeDir("separate");
    {
        RankingStore r(dir.string());
        r.createPlayer(222);
        zowi::SessionStore session("ZowiDesktop", "ZowiApp", dir.string());
        session.setString("activeZowiName", "R2");
        for (const auto &k : session.keys()) session.removeKey(k);  // wipe the whole session
    }
    assert(fs::exists(dir / "ZowiRanking.json"));
    assert(fs::exists(dir / "ZowiApp.json"));
    RankingStore again(dir.string());
    assert(again.hasPlayer(222));
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_online_registration() {
    std::cout << "test_online_registration: " << std::flush;
    fs::path dir = TempStore::makeDir("online");
    {
        RankingStore r(dir.string());
        assert(r.onlineNumber() == 0 && r.onlineToken().empty());
        r.setOnlineRegistration(50, "tok");            // invalid number: ignored
        r.setOnlineRegistration(200, "");              // empty token: ignored
        assert(r.onlineNumber() == 0);
        r.createPlayer(200);
        r.setOnlineRegistration(200, "secret-token");
        assert(r.onlineNumber() == 200 && r.onlineToken() == "secret-token");
    }
    {
        RankingStore r(dir.string());                   // persists across runs
        assert(r.onlineNumber() == 200 && r.onlineToken() == "secret-token");
        assert(r.renamePlayer(200, 250));               // the token belongs to the old number
        assert(r.onlineNumber() == 0 && r.onlineToken().empty());
        r.setOnlineRegistration(250, "t2");
        r.clearOnlineRegistration();
        assert(r.onlineNumber() == 0);
        r.setOnlineRegistration(250, "t3");
        r.clear();                                       // admin clear drops it too
        assert(r.onlineNumber() == 0);
    }
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_file_is_owner_only() {
    std::cout << "test_file_is_owner_only: " << std::flush;
#ifndef _WIN32
    fs::path dir = TempStore::makeDir("perms");
    {
        RankingStore r(dir.string());
        r.createPlayer(222);
        r.setOnlineRegistration(222, "secret");
    }
    const auto perms = fs::status(dir / "ZowiRanking.json").permissions();
    assert((perms & (fs::perms::group_all | fs::perms::others_all)) == fs::perms::none);
    assert((perms & fs::perms::owner_read) != fs::perms::none);
    fs::remove_all(dir);
#endif
    std::cout << "OK" << std::endl;
}

int main() {
    test_numbers_and_names();
    test_create_players();
    test_suggest_free_number();
    test_scoring_and_normalisation();
    test_ranking_order_and_ties();
    test_rename_and_remove();
    test_persistence_and_clear();
    test_corrupt_and_legacy_data();
    test_separate_file_from_session();
    test_online_registration();
    test_file_is_owner_only();
    std::cout << "All ranking_store tests passed." << std::endl;
    return 0;
}
