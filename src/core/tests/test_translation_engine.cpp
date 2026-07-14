#include <zowi/translation_engine.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>

void test_translate_no_file() {
    std::cout << "test_translate_no_file: " << std::flush;

    zowi::TranslationEngine engine;
    engine.load("nonexistent_locale");

    assert(engine.translate("WelcomeScreen.qml", "ZOWI") == "ZOWI");
    assert(engine.currentLocale() == "nonexistent_locale");

    std::cout << "OK" << std::endl;
}

void test_translate_json_parsing() {
    std::cout << "test_translate_json_parsing: " << std::flush;

    std::string tmpDir = std::tmpnam(nullptr);
    std::filesystem::create_directories(tmpDir + "/i18n");

    {
        std::ofstream f(tmpDir + "/i18n/zowi_testLocale.json");
        f << R"({
  "WelcomeScreen.qml": {
    "zowi": "ZOWI",
    "tagline": "Tu compaRobot amigable"
  },
  "SplashScreen.qml": {
    "continue": "Continuar",
    "empty_translation": "Empty translation"
  }
})";
    }

    zowi::TranslationEngine engine;
    engine.setResourceBasePath(tmpDir);
    engine.load("testLocale");

    assert(engine.currentLocale() == "testLocale");
    assert(engine.translate("WelcomeScreen.qml", "zowi") == "ZOWI");
    assert(engine.translate("WelcomeScreen.qml", "tagline") == "Tu compaRobot amigable");
    assert(engine.translate("SplashScreen.qml", "continue") == "Continuar");
    assert(engine.translate("SplashScreen.qml", "empty_translation") == "Empty translation");
    assert(engine.translate("WelcomeScreen.qml", "Unknown") == "Unknown");
    assert(engine.translate("UnknownScreen.qml", "zowi") == "zowi");

    std::filesystem::remove_all(tmpDir);
    std::cout << "OK" << std::endl;
}

void test_available_locales() {
    std::cout << "test_available_locales: " << std::flush;

    zowi::TranslationEngine engine;
    auto locales = engine.availableLocales();
    assert(locales.size() == 5);
    assert(locales[0] == "es_ES");
    assert(locales[1] == "ca_ES");
    assert(locales[2] == "en_US");
    assert(locales[3] == "fr_FR");
    assert(locales[4] == "bg_BG");

    std::cout << "OK" << std::endl;
}

void test_callback() {
    std::cout << "test_callback: " << std::flush;

    zowi::TranslationEngine engine;
    bool called = false;
    engine.onChanged([&]() { called = true; });
    engine.load("nonexistent");
    assert(called == true);

    std::cout << "OK" << std::endl;
}

void test_reload_replaces_translations() {
    std::cout << "test_reload_replaces_translations: " << std::flush;

    static int counter = 0;
    std::string tmpDir = std::string(std::tmpnam(nullptr)) + "_reload_" + std::to_string(counter++);
    std::filesystem::create_directories(tmpDir + "/i18n");

    {
        std::ofstream f(tmpDir + "/i18n/zowi_es_ES.json");
        f << R"({ "Test": { "hello": "Hola" } })";
    }
    {
        std::ofstream f(tmpDir + "/i18n/zowi_ca_ES.json");
        f << R"({ "Test": { "hello": "Hola!" } })";
    }

    zowi::TranslationEngine engine;
    engine.setResourceBasePath(tmpDir);

    engine.load("es_ES");
    assert(engine.translate("Test", "hello") == "Hola");

    engine.load("ca_ES");
    assert(engine.translate("Test", "hello") == "Hola!");
    assert(engine.translate("Test", "nonexistent") == "nonexistent");

    std::filesystem::remove_all(tmpDir);
    std::cout << "OK" << std::endl;
}

int main() {
    test_translate_no_file();
    test_translate_json_parsing();
    test_available_locales();
    test_callback();
    test_reload_replaces_translations();

    std::cout << "\nAll TranslationEngine tests passed!" << std::endl;
    return 0;
}
