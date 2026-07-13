#pragma once

#include <QObject>
#include <zowi/config_store.h>

class ConfigController : public QObject
{
    Q_OBJECT

public:
    explicit ConfigController(QObject *parent = nullptr);

    Q_INVOKABLE QString get(const QString &key) const;

private:
    zowi::ConfigStore m_store;
};
