#include <zowi/translation_engine.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

namespace zowi {

TranslationEngine::TranslationEngine()
    : m_log([](LogLevel, const std::string &) {})
{
    m_loader = [this](const std::string &locale) { return defaultLoader(locale); };
}

namespace {
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
parseJson(const std::string &content) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map;
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(content);
    } catch (const std::exception &) {
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

std::string readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open())
        return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
} // namespace

// Default translation loader: reads "i18n/zowi_<locale>.json" from the
// filesystem under the resource base path (or the current working directory
// when no base path is set). The Qt resource fallback (":/i18n/") lives in the
// host adapters' overrides (GUI/CLI), not here, so core stays framework-free.
std::string TranslationEngine::defaultLoader(const std::string &locale) const {
    const std::string name = "zowi_" + locale + ".json";
    const std::string fs = m_resourceBasePath.empty()
        ? ("i18n/" + name)
        : (m_resourceBasePath + "/i18n/" + name);
    return readFile(fs);
}

void TranslationEngine::load(const std::string &locale) {
    m_translations.clear();
    m_fallback.clear();
    m_currentLocale = locale;

    m_translations = parseJson(m_loader(locale));

    // Always keep English as a fallback so missing translations degrade
    // gracefully instead of showing the raw key.
    if (locale != "en_US") {
        m_fallback = parseJson(m_loader("en_US"));
    }

    m_log(LogLevel::Info, "[i18n] Loaded " + std::to_string(m_translations.size())
        + " contexts for " + locale
        + (m_translations.empty() ? " (EMPTY!)" : " (OK)"));

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
    return {"es_ES", "ca_ES", "en_US", "fr_FR", "bg_BG"};
}

} // namespace zowi