#include "ConfigController.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

ConfigController::ConfigController(QObject *parent)
    : QObject(parent)
{
    load(":/src/config.json");
}

QString ConfigController::get(const QString &key) const
{
    return m_config.value(key).toString();
}

void ConfigController::load(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "ConfigController: cannot open" << path;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isObject())
        m_config = doc.object().toVariantMap();

    qDebug() << "Config loaded:" << m_config;
}
