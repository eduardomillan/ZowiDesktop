#include "TranslatorController.h"
#include <QDebug>
#include <QFile>
#include <QString>
#include <QByteArray>

namespace {

// Qt-aware translation loader: keeps core framework-free. Prefers a filesystem
// copy (dev edits / hot-reload), then falls back to the compiled-in Qt resource
// for packaged builds where the i18n/ directory is not shipped beside the
// binary. This mirrors the resolver the core used before the Qt-free refactor.
zowi::TranslationEngine::TranslationLoader qtTranslationLoader(const QString &basePath)
{
    return [basePath](const std::string &locale) {
        const QString name = QString::fromLatin1("zowi_%1.json").arg(QString::fromStdString(locale));
        const QString fs = basePath.isEmpty()
            ? (QStringLiteral("i18n/") + name)
            : (basePath + QStringLiteral("/i18n/") + name);

        QString path;
        if (QFile::exists(fs))
            path = fs;
        else
            path = QStringLiteral(":/i18n/") + name;

        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            return std::string();
        return file.readAll().toStdString();
    };
}

} // namespace

TranslatorController::TranslatorController(QObject *parent)
    : QObject(parent)
{
    m_engine.setTranslationLoader(qtTranslationLoader(QString()));
    m_engine.setLogCallback(
        [](zowi::TranslationEngine::LogLevel level, const std::string &message) {
            const QString msg = QString::fromStdString(message);
            if (level == zowi::TranslationEngine::LogLevel::Warning)
                qWarning() << msg;
            else
                qInfo() << msg;
        });
    m_engine.onChanged([this]() { emit languageChanged(); });
}

void TranslatorController::load(const QString &locale)
{
    m_engine.load(locale.toStdString());
}

QStringList TranslatorController::availableLocales() const
{
    QStringList result;
    for (const auto &loc : m_engine.availableLocales())
        result.append(QString::fromStdString(loc));
    return result;
}

QString TranslatorController::translate(const QString &context, const QString &source) const
{
    return QString::fromStdString(
        m_engine.translate(context.toStdString(), source.toStdString()));
}

QString TranslatorController::currentLocale() const
{
    return QString::fromStdString(m_engine.currentLocale());
}
