#include "TranslatorController.h"

TranslatorController::TranslatorController(QObject *parent)
    : QObject(parent)
{
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
