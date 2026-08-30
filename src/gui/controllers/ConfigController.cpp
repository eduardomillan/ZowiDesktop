#include "ConfigController.h"
#include <QFile>
#include <QDebug>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent)
    , m_devOverlayVisible(false)
{
    QFile file(":/src/config.json");
    if (file.open(QIODevice::ReadOnly)) {
        std::string jsonStr = file.readAll().toStdString();
        m_store.loadFromString(jsonStr);
        file.close();
    } else {
        qWarning() << "ConfigController: cannot open config.json";
    }
    m_devOverlayVisible = devMode();
}

bool ConfigController::devMode() const
{
    QString env = qEnvironmentVariable("ZOWI_DEV");
    if (!env.isEmpty()) {
        static const QStringList truthy = {"1", "true", "on"};
        return truthy.contains(env.trimmed().toLower());
    }
    QString value = QString::fromStdString(m_store.get("dev_mode"));
    return value.compare("true", Qt::CaseInsensitive) == 0
        || value == "1";
}

QString ConfigController::get(const QString &key) const
{
    return QString::fromStdString(m_store.get(key.toStdString()));
}

bool ConfigController::devOverlayVisible() const
{
    return m_devOverlayVisible;
}

void ConfigController::setDevOverlayVisible(bool visible)
{
    if (m_devOverlayVisible != visible) {
        m_devOverlayVisible = visible;
        emit devOverlayVisibleChanged();
    }
}
