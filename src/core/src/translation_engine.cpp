#include <zowi/translation_engine.h>
#include <fstream>
#include <sstream>

namespace zowi {

TranslationEngine::TranslationEngine() = default;

void TranslationEngine::load(const std::string &locale) {
    m_translations.clear();
    m_currentLocale = locale;

    std::string path = m_resourceBasePath + "/i18n/zowi_" + locale + ".ts";
    if (m_resourceBasePath.empty()) {
        path = "i18n/zowi_" + locale + ".ts";
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        if (m_onChanged) m_onChanged();
        return;
    }

    std::string line;
    std::string currentContext;
    std::string currentSource;

    while (std::getline(file, line)) {
        size_t pos = 0;
        while (pos < line.size()) {
            // Skip whitespace
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t' || line[pos] == '\r'))
                ++pos;
            if (pos >= line.size()) break;

            // Find next '<'
            size_t tagStart = line.find('<', pos);
            if (tagStart == std::string::npos) break;

            // Find closing '>'
            size_t tagEnd = line.find('>', tagStart);
            if (tagEnd == std::string::npos) break;

            std::string tag = line.substr(tagStart + 1, tagEnd - tagStart - 1);

            // Check if it's a closing tag
            bool isClosing = !tag.empty() && tag[0] == '/';
            std::string tagName = isClosing ? tag.substr(1) : tag;

            // Strip attributes (take only the tag name)
            size_t spacePos = tagName.find(' ');
            if (spacePos != std::string::npos)
                tagName = tagName.substr(0, spacePos);

            if (tagName == "context") {
                currentContext.clear();
                currentSource.clear();
            } else if (tagName == "name" && !isClosing) {
                // Extract text between <name> and </name>
                size_t textStart = tagEnd + 1;
                size_t closeTag = line.find("</name>", textStart);
                if (closeTag != std::string::npos) {
                    currentContext = line.substr(textStart, closeTag - textStart);
                }
            } else if (tagName == "source" && !isClosing) {
                size_t textStart = tagEnd + 1;
                size_t closeTag = line.find("</source>", textStart);
                if (closeTag != std::string::npos) {
                    currentSource = line.substr(textStart, closeTag - textStart);
                }
            } else if (tagName == "translation" && !isClosing) {
                size_t textStart = tagEnd + 1;
                size_t closeTag = line.find("</translation>", textStart);
                if (closeTag != std::string::npos) {
                    std::string translation = line.substr(textStart, closeTag - textStart);
                    if (!currentContext.empty() && !currentSource.empty()
                        && !translation.empty() && translation != "...") {
                        m_translations[currentContext][currentSource] = translation;
                    }
                }
            }

            pos = tagEnd + 1;
        }
    }

    if (m_onChanged) m_onChanged();
}

std::string TranslationEngine::translate(const std::string &context, const std::string &source) const {
    auto ctxIt = m_translations.find(context);
    if (ctxIt != m_translations.end()) {
        auto srcIt = ctxIt->second.find(source);
        if (srcIt != ctxIt->second.end()) {
            return srcIt->second;
        }
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
