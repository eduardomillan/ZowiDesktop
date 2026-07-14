#include <zowi/translation_engine.h>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace zowi {

TranslationEngine::TranslationEngine() = default;

namespace {
// Read a JSON translation file of the form:
//   { "Context.qml": { "key": "value", ... }, ... }
// Returns an empty map if the file is missing or malformed.
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
readJson(const std::string &path) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map;
    std::ifstream file(path);
    if (!file.is_open())
        return map;

    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return map;
    }

    if (!j.is_object())
        return map;

    for (auto ctxIt = j.begin(); ctxIt != j.end(); ++ctxIt) {
        const auto &entries = ctxIt.value();
        if (!entries.is_object())
            continue;
        for (auto eIt = entries.begin(); eIt != entries.end(); ++eIt) {
            if (eIt.value().is_string())
                map[ctxIt.key()][eIt.key()] = eIt.value().get<std::string>();
        }
    }
    return map;
}
} // namespace

void TranslationEngine::load(const std::string &locale) {
    m_translations.clear();
    m_fallback.clear();
    m_currentLocale = locale;

    std::string path = m_resourceBasePath.empty()
        ? "i18n/zowi_" + locale + ".json"
        : m_resourceBasePath + "/i18n/zowi_" + locale + ".json";

    m_translations = readJson(path);

    // Always keep English as a fallback so missing translations degrade
    // gracefully instead of showing the raw key.
    if (locale != "en_US") {
        std::string enPath = m_resourceBasePath.empty()
            ? "i18n/zowi_en_US.json"
            : m_resourceBasePath + "/i18n/zowi_en_US.json";
        m_fallback = readJson(enPath);
    }

    if (m_onChanged) m_onChanged();
}

std::string TranslationEngine::translate(const std::string &context, const std::string &source) const {
    auto ctxIt = m_translations.find(context);
    if (ctxIt != m_translations.end()) {
        auto srcIt = ctxIt->second.find(source);
        if (srcIt != ctxIt->second.end())
            return srcIt->second;
    }
    auto fctxIt = m_fallback.find(context);
    if (fctxIt != m_fallback.end()) {
        auto fsrcIt = fctxIt->second.find(source);
        if (fsrcIt != fctxIt->second.end())
            return fsrcIt->second;
    }
    return source;
}

std::string TranslationEngine::currentLocale() const {
    return m_currentLocale;
}

std::vector<std::string> TranslationEngine::availableLocales() const {
    return {"es_ES", "ca_ES", "en_US"};
}

} // namespace zowi
