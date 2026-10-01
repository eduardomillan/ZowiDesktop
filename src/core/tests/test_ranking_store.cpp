#include <zowi/ranking_store.h>

#include <cassert>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using zowi::RankingGame;

namespace {

struct TempStore {
    fs::path dir;
    zowi::RankingStore ranking;

    static fs::path makeDir(const char *name) {
        fs::path d = fs::temp_directory_path() / (std::string("zowi_ranking_test_") + name);
        fs::remove_all(d);
        fs::create_directories(d);
        return d;
    }
    explicit TempStore(const char *name)
        : dir(makeDir(name)), ranking(dir.string()) {}
    ~TempStore() { fs::remove_all(dir); }
};

} // namespace

void test_empty() {
    std::cout << "test_empty: " << std::flush;
    TempStore t("empty");
    assert(t.ranking.top(RankingGame::Mouths).empty());
    assert(t.ranking.best(RankingGame::Mouths) == 0);
    std::cout << "OK" << std::endl;
}

void test_min_score_gate() {
    std::cout << "test_min_score_gate: " << std::flush;
    TempStore t("gate");
    assert(!t.ranking.qualifies(RankingGame::ZowiSays, 2));
    assert(t.ranking.submit(RankingGame::ZowiSays, "Ana", 2) == 0);
    assert(t.ranking.top(RankingGame::ZowiSays).empty());
    assert(t.ranking.qualifies(RankingGame::ZowiSays, 3));
    assert(!t.ranking.qualifies(RankingGame::Mouths, 1));
    assert(t.ranking.qualifies(RankingGame::Mouths, 2));
    std::cout << "OK" << std::endl;
}

void test_order_and_ties() {
    std::cout << "test_order_and_ties: " << std::flush;
    TempStore t("order");
    assert(t.ranking.submit(RankingGame::Mouths, "A", 5, 100) == 1);
    assert(t.ranking.submit(RankingGame::Mouths, "B", 9, 101) == 1);
    assert(t.ranking.submit(RankingGame::Mouths, "C", 5, 102) == 3);  // tie goes after existing 5
    const auto top = t.ranking.top(RankingGame::Mouths);
    assert(top.size() == 3);
    assert(top[0].playerName == "B" && top[0].points == 9 && top[0].timestamp == 101);
    assert(top[1].playerName == "A");
    assert(top[2].playerName == "C");
    assert(t.ranking.best(RankingGame::Mouths) == 9);
    std::cout << "OK" << std::endl;
}

void test_capped_at_ten() {
    std::cout << "test_capped_at_ten: " << std::flush;
    TempStore t("cap");
    for (int i = 0; i < 10; ++i) t.ranking.submit(RankingGame::Timeline, "P" + std::to_string(i), 10 + i, 1);
    assert(t.ranking.top(RankingGame::Timeline).size() == 10);
    // Equal to the lowest (10) does not qualify; higher does and evicts it.
    assert(!t.ranking.qualifies(RankingGame::Timeline, 10));
    assert(t.ranking.submit(RankingGame::Timeline, "Low", 10, 1) == 0);
    assert(t.ranking.submit(RankingGame::Timeline, "New", 11, 1) == 10);
    const auto top = t.ranking.top(RankingGame::Timeline);
    assert(top.size() == 10);
    assert(top.back().points == 11 && top.back().playerName == "New");
    std::cout << "OK" << std::endl;
}

void test_name_sanitizing() {
    std::cout << "test_name_sanitizing: " << std::flush;
    TempStore t("name");
    t.ranking.submit(RankingGame::Mouths, "   ", 5, 1);
    t.ranking.submit(RankingGame::Mouths, "  ABCDEFGHIJKLMNOP  ", 6, 1);
    const auto top = t.ranking.top(RankingGame::Mouths);
    assert(top[0].playerName == "ABCDEFGHIJKL");
    assert(top[1].playerName == "?");
    std::cout << "OK" << std::endl;
}

void test_persistence_and_clear() {
    std::cout << "test_persistence_and_clear: " << std::flush;
    fs::path dir = TempStore::makeDir("persist");
    {
        zowi::RankingStore r(dir.string());
        r.submit(RankingGame::ZowiSays, "Zed", 7, 5);
        r.submit(RankingGame::Mouths, "Mo", 4, 5);
    }
    {
        zowi::RankingStore r(dir.string());
        assert(r.top(RankingGame::ZowiSays).size() == 1);
        assert(r.best(RankingGame::ZowiSays) == 7);
        r.clear(RankingGame::ZowiSays);
        assert(r.top(RankingGame::ZowiSays).empty());
        assert(r.top(RankingGame::Mouths).size() == 1);
        r.clearAll();
        assert(r.top(RankingGame::Mouths).empty());
    }
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_corrupt_data() {
    std::cout << "test_corrupt_data: " << std::flush;
    fs::path dir = TempStore::makeDir("corrupt");
    {
        // Write garbage under a ranking key directly in the ranking file.
        zowi::SessionStore raw("ZowiDesktop", "ZowiRanking", dir.string());
        raw.setString(zowi::RankingStore::keyFor(RankingGame::Mouths), "not json");
    }
    zowi::RankingStore r(dir.string());
    assert(r.top(RankingGame::Mouths).empty());
    assert(r.submit(RankingGame::Mouths, "Ok", 5, 1) == 1);
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_separate_file_from_session() {
    std::cout << "test_separate_file_from_session: " << std::flush;
    fs::path dir = TempStore::makeDir("separate");
    {
        zowi::RankingStore r(dir.string());
        r.submit(RankingGame::Mouths, "Ana", 5, 1);
        zowi::SessionStore session("ZowiDesktop", "ZowiApp", dir.string());
        session.setString("activeZowiName", "R2");
        // Wiping the whole session must not affect rankings.
        for (const auto &k : session.keys()) session.removeKey(k);
    }
    assert(fs::exists(dir / "ZowiRanking.json"));
    assert(fs::exists(dir / "ZowiApp.json"));
    zowi::RankingStore again(dir.string());
    assert(again.top(RankingGame::Mouths).size() == 1);
    zowi::SessionStore sessionAgain("ZowiDesktop", "ZowiApp", dir.string());
    assert(sessionAgain.keys().empty());
    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

int main() {
    test_empty();
    test_min_score_gate();
    test_order_and_ties();
    test_capped_at_ten();
    test_name_sanitizing();
    test_persistence_and_clear();
    test_corrupt_data();
    test_separate_file_from_session();
    std::cout << "All ranking_store tests passed." << std::endl;
    return 0;
}
