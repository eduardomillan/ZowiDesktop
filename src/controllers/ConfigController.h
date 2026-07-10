#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QVariantMap>

class ConfigController : public QObject
{
    Q_OBJECT

public:
    explicit ConfigController(QObject *parent = nullptr);

    Q_INVOKABLE QString get(const QString &key) const;

private:
    void load(const QString &path);
    QVariantMap m_config;
};

#endif
