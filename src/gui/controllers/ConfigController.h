#pragma once

#include <QObject>
#include <zowi/config_store.h>

class ConfigController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool devMode READ devMode CONSTANT)
    Q_PROPERTY(bool devOverlayVisible READ devOverlayVisible WRITE setDevOverlayVisible NOTIFY devOverlayVisibleChanged)

public:
    explicit ConfigController(QObject *parent = nullptr);

    Q_INVOKABLE QString get(const QString &key) const;
    bool devMode() const;

    bool devOverlayVisible() const;
    void setDevOverlayVisible(bool visible);

signals:
    void devOverlayVisibleChanged();

private:
    zowi::ConfigStore m_store;
    bool m_devOverlayVisible;
};
