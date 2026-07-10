#ifndef TRANSLATORCONTROLLER_H
#define TRANSLATORCONTROLLER_H

#include <QObject>

class TranslationEngine;

class TranslatorController : public QObject
{
    Q_OBJECT

public:
    explicit TranslatorController(TranslationEngine *engine, QObject *parent = nullptr);

    Q_INVOKABLE void load(const QString &locale);
    Q_INVOKABLE QStringList availableLocales() const;
    Q_INVOKABLE QString translate(const QString &context, const QString &source) const;
    Q_INVOKABLE QString currentLocale() const;

signals:
    void languageChanged();

private:
    TranslationEngine *m_engine;
};

#endif
