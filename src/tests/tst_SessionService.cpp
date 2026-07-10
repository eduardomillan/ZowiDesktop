#include <QtTest>
#include <QtCore>
#include "services/SessionService.h"

class tst_SessionService : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Use a separate settings file for tests
        QCoreApplication::setOrganizationName("ZowiDesktop_test");
        QCoreApplication::setApplicationName("ZowiApp_test");
    }

    void cleanup()
    {
        QSettings s("ZowiDesktop_test", "ZowiApp_test");
        s.clear();
    }

    void testDefaults()
    {
        SessionService svc;
        QCOMPARE(svc.activeZowiDeviceAddress(), QString());
        QCOMPARE(svc.activeZowiName(), QStringLiteral("Zowi"));
        QCOMPARE(svc.wizardDismissed(), false);
    }

    void testSaveAndLoadAddress()
    {
        SessionService svc;
        QSignalSpy spy(&svc, &SessionService::sessionChanged);

        svc.setActiveZowiDeviceAddress("00:11:22:33:44:55");
        QCOMPARE(spy.count(), 1);
        QCOMPARE(svc.activeZowiDeviceAddress(), QStringLiteral("00:11:22:33:44:55"));
    }

    void testSanitizeName_empty()
    {
        SessionService svc;
        svc.setActiveZowiName("   ");
        QCOMPARE(svc.activeZowiName(), QStringLiteral("Zowi"));
    }

    void testSanitizeName_booleanStrings()
    {
        SessionService svc;
        svc.setActiveZowiName("false");
        QCOMPARE(svc.activeZowiName(), QStringLiteral("Zowi"));

        svc.setActiveZowiName("true");
        QCOMPARE(svc.activeZowiName(), QStringLiteral("Zowi"));
    }

    void testSanitizeName_valid()
    {
        SessionService svc;
        svc.setActiveZowiName("MyZowi");
        QCOMPARE(svc.activeZowiName(), QStringLiteral("MyZowi"));
    }

    void testWizardDismissed()
    {
        SessionService svc;
        QSignalSpy spy(&svc, &SessionService::sessionChanged);

        svc.setWizardDismissed(true);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(svc.wizardDismissed(), true);

        svc.setWizardDismissed(false);
        QCOMPARE(svc.wizardDismissed(), false);
    }

    void testPersistenceBetweenInstances()
    {
        {
            SessionService svc;
            svc.setActiveZowiDeviceAddress("AA:BB:CC:DD:EE:FF");
            svc.setActiveZowiName("Robot");
            svc.setWizardDismissed(true);
        }
        {
            SessionService svc;
            QCOMPARE(svc.activeZowiDeviceAddress(), QStringLiteral("AA:BB:CC:DD:EE:FF"));
            QCOMPARE(svc.activeZowiName(), QStringLiteral("Robot"));
            QCOMPARE(svc.wizardDismissed(), true);
        }
    }
};

QTEST_MAIN(tst_SessionService)
#include "tst_SessionService.moc"
