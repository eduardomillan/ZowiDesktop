#include <QColor>
#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickView>
#include <QTimer>
#include <QUrl>

#include "controllers/ConfigController.h"
#include "controllers/TranslatorController.h"

class PreviewSession final : public QObject
{
    Q_OBJECT

public:
    explicit PreviewSession(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE QString loadActiveZowiDeviceAddress() const { return m_address; }
    Q_INVOKABLE QString loadActiveZowiName() const { return m_name; }
    Q_INVOKABLE bool hasDismissedWizard() const { return m_wizardDismissed; }

    Q_INVOKABLE void saveActiveZowiDeviceAddress(const QString &address) { m_address = address; }
    Q_INVOKABLE void saveActiveZowiName(const QString &name) { m_name = name; }
    Q_INVOKABLE void saveWizardDismissed(bool dismissed) { m_wizardDismissed = dismissed; }

private:
    QString m_address;
    QString m_name;
    bool m_wizardDismissed = false;
};

class PreviewBluetooth final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)
    Q_PROPERTY(bool scanning READ isScanning NOTIFY scanningChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY deviceChanged)
    Q_PROPERTY(QString deviceAddress READ deviceAddress NOTIFY deviceChanged)

public:
    explicit PreviewBluetooth(QObject *parent = nullptr) : QObject(parent) {}

    bool isConnected() const { return m_connected; }
    bool isScanning() const { return m_scanning; }
    QString deviceName() const { return m_deviceName; }
    QString deviceAddress() const { return m_deviceAddress; }

    void setInitialConnection(const QString &name, const QString &address)
    {
        m_connected = true;
        m_deviceName = name;
        m_deviceAddress = address;
    }

    Q_INVOKABLE void startScan()
    {
        if (m_scanning)
            return;

        m_scanning = true;
        emit scanningChanged();

        QTimer::singleShot(250, this, [this]() {
            emit deviceDiscovered(QStringLiteral("Zowi Demo"), QStringLiteral("B4:9D:0B:32:41:0E"));
        });
        QTimer::singleShot(450, this, [this]() {
            emit deviceDiscovered(QStringLiteral("Zowi Lab"), QStringLiteral("B4:9D:0B:32:41:0F"));
        });
        QTimer::singleShot(700, this, [this]() {
            stopScan();
        });
    }

    Q_INVOKABLE void stopScan()
    {
        if (!m_scanning)
            return;

        m_scanning = false;
        emit scanningChanged();
        emit scanFinished();
    }

    Q_INVOKABLE void connectToDevice(const QString &address)
    {
        m_deviceAddress = address;
        emit deviceChanged();

        QTimer::singleShot(300, this, [this, address]() {
            m_connected = true;
            m_deviceName = address.endsWith(QLatin1Char('F'))
                ? QStringLiteral("Zowi Lab")
                : QStringLiteral("Zowi Demo");
            emit deviceChanged();
            emit connectionChanged();
        });
    }

    Q_INVOKABLE void disconnectFromDevice()
    {
        m_connected = false;
        m_deviceName.clear();
        m_deviceAddress.clear();
        emit deviceChanged();
        emit connectionChanged();
    }

    Q_INVOKABLE void sendData(const QString &) {}

signals:
    void deviceDiscovered(const QString &name, const QString &address);
    void scanFinished();
    void connectionChanged();
    void scanningChanged();
    void deviceChanged();
    void dataReceived(const QString &data);
    void errorOccurred(const QString &message);

private:
    bool m_connected = false;
    bool m_scanning = false;
    QString m_deviceName;
    QString m_deviceAddress;
};

struct PreviewOptions
{
    QString screenPath;
    QString locale = QStringLiteral("en_US");
    QString deviceName = QStringLiteral("Zowi Demo");
    QString deviceAddress = QStringLiteral("B4:9D:0B:32:41:0E");
    bool connected = false;
};

static PreviewOptions parseOptions(const QStringList &arguments)
{
    PreviewOptions options;

    for (int i = 1; i < arguments.size(); ++i) {
        const QString arg = arguments.at(i);
        if (arg == QStringLiteral("--connected")) {
            options.connected = true;
        } else if (arg.startsWith(QStringLiteral("--locale="))) {
            options.locale = arg.section(QLatin1Char('='), 1);
        } else if (arg == QStringLiteral("--locale") && i + 1 < arguments.size()) {
            options.locale = arguments.at(++i);
        } else if (arg.startsWith(QStringLiteral("--device-name="))) {
            options.deviceName = arg.section(QLatin1Char('='), 1);
        } else if (arg == QStringLiteral("--device-name") && i + 1 < arguments.size()) {
            options.deviceName = arguments.at(++i);
        } else if (arg.startsWith(QStringLiteral("--device-address="))) {
            options.deviceAddress = arg.section(QLatin1Char('='), 1);
        } else if (arg == QStringLiteral("--device-address") && i + 1 < arguments.size()) {
            options.deviceAddress = arguments.at(++i);
        } else if (!arg.startsWith(QStringLiteral("--")) && options.screenPath.isEmpty()) {
            options.screenPath = arg;
        }
    }

    return options;
}

static void loadScreen(QQuickView &view, const QString &screenPath)
{
    view.setTitle(QStringLiteral("Zowi Screen Preview - %1").arg(QFileInfo(screenPath).baseName()));
    view.setSource(QUrl::fromLocalFile(screenPath));
}

int main(int argc, char *argv[])
{
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("zowi_screen_preview"));

    const PreviewOptions options = parseOptions(QCoreApplication::arguments());
    if (options.screenPath.isEmpty()) {
        qCritical("Usage: zowi_screen_preview <screen.qml> [--locale xx_YY] [--connected] [--device-name NAME] [--device-address MAC]");
        return 1;
    }

    TranslatorController translator;
    translator.load(options.locale);

    ConfigController config;
    PreviewSession session;
    PreviewBluetooth bluetooth;

    if (options.connected) {
        session.saveActiveZowiDeviceAddress(options.deviceAddress);
        session.saveActiveZowiName(options.deviceName);
        bluetooth.setInitialConnection(options.deviceName, options.deviceAddress);
    }

    QQuickView view;
    view.rootContext()->setContextProperty(QStringLiteral("Translator"), &translator);
    view.rootContext()->setContextProperty(QStringLiteral("Config"), &config);
    view.rootContext()->setContextProperty(QStringLiteral("Session"), &session);
    view.rootContext()->setContextProperty(QStringLiteral("Bluetooth"), &bluetooth);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setWidth(1024);
    view.setHeight(600);
    view.setColor(QColor(QStringLiteral("#f4f9f4")));

    QObject::connect(&translator, &TranslatorController::languageChanged, [&view, options]() {
        loadScreen(view, options.screenPath);
    });

    loadScreen(view, options.screenPath);
    if (view.status() != QQuickView::Ready)
        return 1;

    view.show();
    return app.exec();
}

#include "preview_main.moc"
