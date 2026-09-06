#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace zowi {

class TranslationEngine {
public:
    TranslationEngine();

    // Diagnostics severity.
    enum class LogLevel { Info, Warning };

    // Locale JSON provider. Given a locale (e.g. "es_ES"), returns the raw JSON
    // document for that locale, or an empty string when unavailable. The default
    // implementation reads "i18n/zowi_<locale>.json" from the filesystem under
    // the resource base path. Host adapters (Qt GUI/CLI) may replace it to also
    // consult an embedded resource bundle (e.g. the ":/i18n/" qrc), which keeps
    // core free of any framework.
    using TranslationLoader = std::function<std::string(const std::string &locale)>;
    void setTranslationLoader(TranslationLoader loader) { m_loader = std::move(loader); }

    // Diagnostics sink; called with a fully formatted message. Defaults to a
    // no-op. Host adapters route messages to their own logging.
    using LogCallback = std::function<void(LogLevel level, const std::string &message)>;
    void setLogCallback(LogCallback cb) { m_log = std::move(cb); }

    void load(const std::string &locale);
    void setResourceBasePath(const std::string &path) { m_resourceBasePath = path; }
    std::string translate(const std::string &context, const std::string &source) const;
    std::string currentLocale() const;
    std::vector<std::string> availableLocales() const;

    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { m_onChanged = std::move(cb); }

private:
    std::string defaultLoader(const std::string &locale) const;

    using TranslationMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;

    TranslationMap m_translations;   // current locale
    TranslationMap m_fallback;       // always English (en_US) when current != en_US
    std::string m_currentLocale;
    std::string m_resourceBasePath;
    TranslationLoader m_loader;
    LogCallback m_log;
    ChangedCallback m_onChanged;
};

} // namespace zowi