#ifndef TRANSLATIONENGINE_H
#define TRANSLATIONENGINE_H

#include <QObject>
#include <QHash>
#include <QStringList>

class TranslationEngine : public QObject
{
    Q_OBJECT

public:
    explicit TranslationEngine(QObject *parent = nullptr);

    void load(const QString &locale);
    QStringList availableLocales() const;
    QString translate(const QString &context, const QString &source) const;
    QString currentLocale() const;

    static void setInstance(TranslationEngine *instance);
    static QString tr(const QString &context, const QString &source);

signals:
    void languageChanged();

private:
    static TranslationEngine *s_instance;
    using TranslationMap = QHash<QString, QHash<QString, QString>>;
    TranslationMap m_translations;
    QString m_currentLocale;
};

#endif
