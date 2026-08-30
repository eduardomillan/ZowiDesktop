#include <zowi/translation_engine.h>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <nlohmann/json.hpp>

namespace zowi {

TranslationEngine::TranslationEngine() = default;

namespace {
std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
readJson(const std::string &path) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> map;
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[i18n] Failed to open" << QString::fromStdString(path)
                    << "- error:" << file.errorString();
        return map;
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(file.readAll().toStdString());
    } catch (const std::exception &e) {
        qWarning() << "[i18n] Failed to parse" << QString::fromStdString(path)
                    << "- exception:" << e.what();
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

// Resolve the location of a locale's JSON. Prefer a filesystem copy so
// development edits / hot-reload keep working without rebuilding the qrc,
// and fall back to the compiled-in Qt resource for packaged builds where
// the i18n/ directory is not present alongside the binary.
QString resolvePath(const std::string &basePath, const std::string &locale) {
    const QString name = QString::fromLatin1("zowi_%1.json").arg(QString::fromStdString(locale));
    const QString fs = basePath.empty()
        ? (QStringLiteral("i18n/") + name)
        : (QString::fromStdString(basePath) + QStringLiteral("/i18n/") + name);
    if (QFile::exists(fs))
        return fs;
    return QStringLiteral(":/i18n/") + name;
}
} // namespace

void TranslationEngine::load(const std::string &locale) {
    m_translations.clear();
    m_fallback.clear();
    m_currentLocale = locale;

    const QString localePath = resolvePath(m_resourceBasePath, locale);
    m_translations = readJson(localePath.toStdString());

    // Always keep English as a fallback so missing translations degrade
    // gracefully instead of showing the raw key.
    if (locale != "en_US") {
        m_fallback = readJson(resolvePath(m_resourceBasePath, "en_US").toStdString());
    }

    qInfo() << "[i18n] Loaded" << m_translations.size() << "contexts for"
            << QString::fromStdString(locale)
            << (m_translations.empty() ? "(EMPTY!)" : "OK");

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
