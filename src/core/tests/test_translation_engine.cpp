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

void test_translate_xml_parsing() {
    std::cout << "test_translate_xml_parsing: " << std::flush;

    std::string tmpDir = std::tmpnam(nullptr);
    std::filesystem::create_directories(tmpDir + "/i18n");

    {
        std::ofstream f(tmpDir + "/i18n/zowi_testLocale.ts");
        f << R"(<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="es_ES">
<context>
    <name>WelcomeScreen.qml</name>
    <message>
        <source>ZOWI</source>
        <translation>ZOWI</translation>
    </message>
    <message>
        <source>Your friendly robot companion</source>
        <translation>Tu compaRobot amigable</translation>
    </message>
</context>
<context>
    <name>SplashScreen.qml</name>
    <message>
        <source>Continuar</source>
        <translation>Continuar</translation>
    </message>
    <message>
        <source>Empty translation</source>
        <translation>...</translation>
    </message>
</context>
</TS>)";
    }

    zowi::TranslationEngine engine;
    engine.setResourceBasePath(tmpDir);
    engine.load("testLocale");

    assert(engine.currentLocale() == "testLocale");
    assert(engine.translate("WelcomeScreen.qml", "ZOWI") == "ZOWI");
    assert(engine.translate("WelcomeScreen.qml", "Your friendly robot companion") == "Tu compaRobot amigable");
    assert(engine.translate("SplashScreen.qml", "Continuar") == "Continuar");
    assert(engine.translate("SplashScreen.qml", "Empty translation") == "Empty translation");
    assert(engine.translate("WelcomeScreen.qml", "Unknown") == "Unknown");
    assert(engine.translate("UnknownScreen.qml", "ZOWI") == "ZOWI");

    std::filesystem::remove_all(tmpDir);
    std::cout << "OK" << std::endl;
}

void test_available_locales() {
    std::cout << "test_available_locales: " << std::flush;

    zowi::TranslationEngine engine;
    auto locales = engine.availableLocales();
    assert(locales.size() == 3);
    assert(locales[0] == "es_ES");
    assert(locales[1] == "ca_ES");
    assert(locales[2] == "en_US");

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
        std::ofstream f(tmpDir + "/i18n/zowi_es_ES.ts");
        f << R"(<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="es_ES">
<context><name>Test</name>
<message><source>Hello</source><translation>Hola</translation></message>
</context></TS>)";
    }
    {
        std::ofstream f(tmpDir + "/i18n/zowi_ca_ES.ts");
        f << R"(<?xml version="1.0" encoding="utf-8"?>
<TS version="2.1" language="ca_ES">
<context><name>Test</name>
<message><source>Hello</source><translation>Hola!</translation></message>
</context></TS>)";
    }

    zowi::TranslationEngine engine;
    engine.setResourceBasePath(tmpDir);

    engine.load("es_ES");
    assert(engine.translate("Test", "Hello") == "Hola");

    engine.load("ca_ES");
    assert(engine.translate("Test", "Hello") == "Hola!");
    assert(engine.translate("Test", "Nonexistent") == "Nonexistent");

    std::filesystem::remove_all(tmpDir);
    std::cout << "OK" << std::endl;
}

int main() {
    test_translate_no_file();
    test_translate_xml_parsing();
    test_available_locales();
    test_callback();
    test_reload_replaces_translations();

    std::cout << "\nAll TranslationEngine tests passed!" << std::endl;
    return 0;
}
