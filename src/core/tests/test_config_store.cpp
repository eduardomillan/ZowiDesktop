#include <zowi/config_store.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>
#include <map>

void test_config_load() {
    std::cout << "test_config_load: " << std::flush;

    std::string tmpPath = std::tmpnam(nullptr);
    tmpPath += ".json";

    {
        std::ofstream f(tmpPath);
        f << R"({
            "know_more": "https://example.com",
            "splash_image": "qrc:/images/test.png",
            "count": 42
        })";
    }

    zowi::ConfigStore store(tmpPath);
    assert(store.get("know_more") == "https://example.com");
    assert(store.get("splash_image") == "qrc:/images/test.png");
    assert(store.get("missing") == "");
    assert(store.has("know_more") == true);
    assert(store.has("missing") == false);

    std::remove(tmpPath.c_str());
    std::cout << "OK" << std::endl;
}

void test_config_empty_file() {
    std::cout << "test_config_empty_file: " << std::flush;

    std::string tmpPath = std::tmpnam(nullptr);
    tmpPath += ".json";

    {
        std::ofstream f(tmpPath);
        f << "{}";
    }

    zowi::ConfigStore store(tmpPath);
    assert(store.get("anything") == "");
    assert(store.has("anything") == false);

    std::remove(tmpPath.c_str());
    std::cout << "OK" << std::endl;
}

void test_config_missing_file() {
    std::cout << "test_config_missing_file: " << std::flush;

    zowi::ConfigStore store("/nonexistent/path.json");
    assert(store.get("key") == "");

    std::cout << "OK" << std::endl;
}

namespace {

struct LayerFiles {
    std::filesystem::path dir;
    std::string systemPath, userPath;

    explicit LayerFiles(const char *name) {
        dir = std::filesystem::temp_directory_path() / (std::string("zowi_config_test_") + name);
        std::filesystem::remove_all(dir);
        std::filesystem::create_directories(dir);
        systemPath = (dir / "system.json").string();
        userPath = (dir / "user.json").string();
    }
    ~LayerFiles() { std::filesystem::remove_all(dir); }

    static void write(const std::string &path, const std::string &text) {
        std::ofstream f(path);
        f << text;
    }
};

const char *kCompiled = R"({
    "ranking_enabled": "true",
    "window_size_ratio": "0.75",
    "log_level": "info",
    "allow_environment_config": "false",
    "allow_user_config": "false"
})";

zowi::ConfigLayers makeLayers(const LayerFiles &files, std::map<std::string, std::string> env = {}) {
    zowi::ConfigLayers layers;
    layers.systemPath = files.systemPath;
    layers.userPath = files.userPath;
    layers.getEnv = [env](const std::string &name) {
        auto it = env.find(name);
        return it == env.end() ? std::string() : it->second;
    };
    return layers;
}

} // namespace

void test_bool_parsing() {
    std::cout << "test_bool_parsing: " << std::flush;
    assert(zowi::ConfigStore::parseBool("true") && zowi::ConfigStore::parseBool("TRUE") && zowi::ConfigStore::parseBool("1"));
    assert(!zowi::ConfigStore::parseBool("false") && !zowi::ConfigStore::parseBool("False") && !zowi::ConfigStore::parseBool("0"));
    assert(zowi::ConfigStore::parseBool("maybe", true) && !zowi::ConfigStore::parseBool("", false));
    assert(zowi::ConfigStore::envNameFor("ranking_enabled") == "ZOWI_RANKING_ENABLED");
    std::cout << "OK" << std::endl;
}

void test_merge_converts_values() {
    std::cout << "test_merge_converts_values: " << std::flush;
    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    assert(store.mergeFromString(R"({"ranking_enabled": false, "message_duration": 2500, "log_level": "warn", "bad": [1,2]})"));
    assert(store.get("ranking_enabled") == "false");   // bool -> text
    assert(store.get("message_duration") == "2500");   // number -> text
    assert(store.get("log_level") == "warn");
    assert(!store.has("bad"));                          // unsupported type skipped
    assert(store.get("window_size_ratio") == "0.75");   // untouched keys stay
    assert(!store.mergeFromString("not json"));
    assert(!store.mergeFromString("[1,2]"));
    std::cout << "OK" << std::endl;
}

void test_defaults_without_files() {
    std::cout << "test_defaults_without_files: " << std::flush;
    LayerFiles files("none");
    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files, {{"ZOWI_RANKING_ENABLED", "false"}}));
    // allow_* are false in the compiled config: environment is ignored
    assert(store.get("ranking_enabled") == "true");
    assert(!store.getBool("allow_user_config", true));
    std::cout << "OK" << std::endl;
}

void test_system_file_wins_and_enables_layers() {
    std::cout << "test_system_file_wins_and_enables_layers: " << std::flush;
    LayerFiles files("system");
    LayerFiles::write(files.systemPath, R"({"allow_environment_config": true, "allow_user_config": "true", "log_level": "warn"})");
    LayerFiles::write(files.userPath, R"({"window_size_ratio": "0.5", "log_level": "debug", "ranking_enabled": "false"})");

    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files, {{"ZOWI_WINDOW_SIZE_RATIO", "0.9"}, {"ZOWI_LOG_LEVEL", "error"}}));

    assert(store.get("log_level") == "warn");           // system beats user and environment
    assert(store.get("window_size_ratio") == "0.9");    // environment beats user
    assert(store.get("ranking_enabled") == "false");    // user beats compiled
    assert(store.getBool("allow_user_config") && store.getBool("allow_environment_config"));
    std::cout << "OK" << std::endl;
}

void test_per_layer_enabling() {
    std::cout << "test_per_layer_enabling: " << std::flush;
    LayerFiles files("enabling");
    LayerFiles::write(files.systemPath, R"({"allow_user_config": "true"})");   // environment stays off
    LayerFiles::write(files.userPath, R"({"window_size_ratio": "0.5"})");

    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files, {{"ZOWI_WINDOW_SIZE_RATIO", "0.9"}}));
    assert(store.get("window_size_ratio") == "0.5");    // user honoured, env not
    std::cout << "OK" << std::endl;
}

void test_system_locks_a_key() {
    std::cout << "test_system_locks_a_key: " << std::flush;
    LayerFiles files("lock");
    LayerFiles::write(files.systemPath, R"({"allow_environment_config": "true", "allow_user_config": "true", "ranking_enabled": "false"})");
    LayerFiles::write(files.userPath, R"({"ranking_enabled": "true"})");

    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files, {{"ZOWI_RANKING_ENABLED", "true"}}));
    assert(store.get("ranking_enabled") == "false");   // fixed by the system file
    std::cout << "OK" << std::endl;
}

void test_allow_switches_only_from_system() {
    std::cout << "test_allow_switches_only_from_system: " << std::flush;
    LayerFiles files("switches");
    // The user file and the environment try to switch the layers on for themselves.
    LayerFiles::write(files.userPath, R"({"allow_user_config": "true", "allow_environment_config": "true", "window_size_ratio": "0.5"})");

    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files, {{"ZOWI_ALLOW_USER_CONFIG", "true"}, {"ZOWI_ALLOW_ENVIRONMENT_CONFIG", "true"}, {"ZOWI_WINDOW_SIZE_RATIO", "0.9"}}));
    assert(store.get("window_size_ratio") == "0.75");   // nothing was allowed
    assert(!store.getBool("allow_user_config", true));

    // Allowed by the system file, the user file still cannot change the switches.
    LayerFiles::write(files.systemPath, R"({"allow_user_config": "true", "allow_environment_config": "false"})");
    zowi::ConfigStore second;
    second.loadFromString(kCompiled);
    second.applyOverrides(makeLayers(files));
    assert(second.get("window_size_ratio") == "0.5");
    assert(!second.getBool("allow_environment_config", true));
    std::cout << "OK" << std::endl;
}

void test_corrupt_and_non_object_layers() {
    std::cout << "test_corrupt_and_non_object_layers: " << std::flush;
    LayerFiles files("corrupt");
    LayerFiles::write(files.systemPath, "{ not json");
    LayerFiles::write(files.userPath, "[1,2,3]");

    zowi::ConfigStore store;
    store.loadFromString(kCompiled);
    store.applyOverrides(makeLayers(files));
    assert(store.get("ranking_enabled") == "true");
    assert(store.get("log_level") == "info");
    std::cout << "OK" << std::endl;
}

void test_default_paths() {
    std::cout << "test_default_paths: " << std::flush;
#ifndef _WIN32
    assert(zowi::ConfigStore::defaultSystemConfigPath() == "/etc/ZowiDesktop/config.json");
    const std::string user = zowi::ConfigStore::defaultUserConfigPath();
    const std::string tail = "/ZowiDesktop/config.json";
    assert(user.size() > tail.size() && user.compare(user.size() - tail.size(), tail.size(), tail) == 0);
#endif
    std::cout << "OK" << std::endl;
}

int main() {
    test_config_load();
    test_config_empty_file();
    test_config_missing_file();
    test_bool_parsing();
    test_merge_converts_values();
    test_defaults_without_files();
    test_system_file_wins_and_enables_layers();
    test_per_layer_enabling();
    test_system_locks_a_key();
    test_allow_switches_only_from_system();
    test_corrupt_and_non_object_layers();
    test_default_paths();

    std::cout << "\nAll ConfigStore tests passed!" << std::endl;
    return 0;
}
