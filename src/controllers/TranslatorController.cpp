#include "TranslatorController.h"
#include "services/TranslationEngine.h"

TranslatorController::TranslatorController(TranslationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    connect(m_engine, &TranslationEngine::languageChanged,
            this, &TranslatorController::languageChanged);
}

void TranslatorController::load(const QString &locale)
{
    m_engine->load(locale);
}

QStringList TranslatorController::availableLocales() const
{
    return m_engine->availableLocales();
}

QString TranslatorController::translate(const QString &context, const QString &source) const
{
    return m_engine->translate(context, source);
}

QString TranslatorController::currentLocale() const
{
    return m_engine->currentLocale();
}
