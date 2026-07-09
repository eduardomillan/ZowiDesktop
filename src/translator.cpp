#include "translator.h"
#include <QFile>
#include <QXmlStreamReader>
#include <QDebug>

Translator *Translator::s_instance = nullptr;

Translator::Translator(QObject *parent)
    : QObject(parent)
{
    s_instance = this;
}

void Translator::setInstance(Translator *instance)
{
    s_instance = instance;
}

QString Translator::tr(const QString &context, const QString &source)
{
    if (s_instance)
        return s_instance->translate(context, source);
    return source;
}

void Translator::load(const QString &locale)
{
    m_translations.clear();
    m_currentLocale = locale;

    QString tsPath = QString(":/i18n/zowi_%1.ts").arg(locale);
    qDebug() << "Loading translations from:" << tsPath;

    QFile file(tsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not load translations for" << locale;
        return;
    }

    qDebug() << "File opened successfully, size:" << file.size();

    QXmlStreamReader xml(&file);
    QString currentContext;

    while (!xml.atEnd() && !xml.hasError()) {
        QXmlStreamReader::TokenType token = xml.readNext();

        if (token == QXmlStreamReader::StartElement) {
            QString elementName = xml.name().toString();

            if (elementName == "context") {
                currentContext.clear();
            } else if (elementName == "name") {
                currentContext = xml.readElementText();
            } else if (elementName == "source") {
                QString source = xml.readElementText();
                if (!currentContext.isEmpty() && !source.isEmpty()) {
                    xml.readNextStartElement();
                    if (xml.name().toString() == "translation") {
                        QString translation = xml.readElementText();
                        if (!translation.isEmpty() && translation != "...") {
                            m_translations[currentContext][source] = translation;
                        }
                    }
                }
            }
        }
    }

    if (xml.hasError()) {
        qWarning() << "XML error:" << xml.errorString();
    }

    file.close();

    qDebug() << "Loaded translations for" << locale << "-" << m_translations.size() << "contexts";

    emit languageChanged();
}

QStringList Translator::availableLocales() const
{
    return {"es_ES", "ca_ES", "en_US"};
}

QString Translator::translate(const QString &context, const QString &source) const
{
    if (m_translations.contains(context) && m_translations[context].contains(source)) {
        return m_translations[context][source];
    }
    return source;
}

QString Translator::currentLocale() const
{
    return m_currentLocale;
}
