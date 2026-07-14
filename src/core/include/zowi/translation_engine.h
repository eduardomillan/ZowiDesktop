#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace zowi {

class TranslationEngine {
public:
    TranslationEngine();

    void load(const std::string &locale);
    void setResourceBasePath(const std::string &path) { m_resourceBasePath = path; }
    std::string translate(const std::string &context, const std::string &source) const;
    std::string currentLocale() const;
    std::vector<std::string> availableLocales() const;

    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { m_onChanged = std::move(cb); }

private:
    using TranslationMap = std::unordered_map<std::string, std::unordered_map<std::string, std::string>>;
    TranslationMap m_translations;   // current locale
    TranslationMap m_fallback;       // always English (en_US) when current != en_US
    std::string m_currentLocale;
    std::string m_resourceBasePath;
    ChangedCallback m_onChanged;
};

} // namespace zowi
