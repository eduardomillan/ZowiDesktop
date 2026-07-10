#ifndef CONFIGCONTROLLER_H
#define CONFIGCONTROLLER_H

#include <QObject>
#include <QVariantMap>

class ConfigController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString knowMoreUrl READ knowMoreUrl CONSTANT)

public:
    explicit ConfigController(QObject *parent = nullptr);

    QString knowMoreUrl() const;

private:
    void load(const QString &path);
    QVariantMap m_config;
};

#endif
