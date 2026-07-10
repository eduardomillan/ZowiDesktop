#include <QtTest>
#include <QtCore>
#include "services/TranslationEngine.h"

class tst_TranslationEngine : public QObject
{
    Q_OBJECT

private slots:
    void testAvailableLocales()
    {
        TranslationEngine engine;
        QStringList locales = engine.availableLocales();
        QVERIFY(locales.contains("es_ES"));
        QVERIFY(locales.contains("ca_ES"));
        QVERIFY(locales.contains("en_US"));
        QCOMPARE(locales.size(), 3);
    }

    void testDefaultLocale()
    {
        TranslationEngine engine;
        QCOMPARE(engine.currentLocale(), QString());
    }

    void testTranslateFallback()
    {
        // Without loading any file, translate should return source string
        TranslationEngine engine;
        QString result = engine.translate("context", "Hello");
        QCOMPARE(result, QStringLiteral("Hello"));
    }

    void testStaticTrFallback()
    {
        // Without instance, static tr returns source
        TranslationEngine::setInstance(nullptr);
        QString result = TranslationEngine::tr("ctx", "Source text");
        QCOMPARE(result, QStringLiteral("Source text"));
    }

    void testTrivialRoundTrip()
    {
        TranslationEngine engine;
        engine.load("en_US");

        // After loading en_US (which may be empty/fallback), translate should work
        QString locale = engine.currentLocale();
        QCOMPARE(locale, QStringLiteral("en_US"));
    }
};

QTEST_MAIN(tst_TranslationEngine)
#include "tst_TranslationEngine.moc"
