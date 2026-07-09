#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QObject>
#include <QHash>
#include <QStringList>

class Translator : public QObject
{
    Q_OBJECT

public:
    explicit Translator(QObject *parent = nullptr);

    Q_INVOKABLE void load(const QString &locale);
    Q_INVOKABLE QStringList availableLocales() const;
    Q_INVOKABLE QString translate(const QString &context, const QString &source) const;
    Q_INVOKABLE QString currentLocale() const;

    static void setInstance(Translator *instance);
    static QString tr(const QString &context, const QString &source);

signals:
    void languageChanged();

private:
    static Translator *s_instance;
    using TranslationMap = QHash<QString, QHash<QString, QString>>;
    TranslationMap m_translations;
    QString m_currentLocale;
};

#endif
