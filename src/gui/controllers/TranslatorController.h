#pragma once

#include <QObject>
#include <QStringList>
#include <zowi/translation_engine.h>

class TranslatorController : public QObject
{
    Q_OBJECT

public:
    explicit TranslatorController(QObject *parent = nullptr);

    Q_INVOKABLE void load(const QString &locale);
    Q_INVOKABLE QStringList availableLocales() const;
    Q_INVOKABLE QString translate(const QString &context, const QString &source) const;
    Q_INVOKABLE QString currentLocale() const;

signals:
    void languageChanged();

private:
    zowi::TranslationEngine m_engine;
};
