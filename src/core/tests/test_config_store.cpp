#include <zowi/config_store.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>

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

int main() {
    test_config_load();
    test_config_empty_file();
    test_config_missing_file();

    std::cout << "\nAll ConfigStore tests passed!" << std::endl;
    return 0;
}
