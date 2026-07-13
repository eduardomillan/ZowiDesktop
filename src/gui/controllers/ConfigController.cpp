#include "ConfigController.h"
#include <QFile>
#include <QDebug>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent)
{
    QFile file(":/src/config.json");
    if (file.open(QIODevice::ReadOnly)) {
        std::string jsonStr = file.readAll().toStdString();
        m_store.loadFromString(jsonStr);
        file.close();
    } else {
        qWarning() << "ConfigController: cannot open config.json";
    }
}

QString ConfigController::get(const QString &key) const
{
    return QString::fromStdString(m_store.get(key.toStdString()));
}
