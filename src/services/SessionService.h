#ifndef SESSIONSERVICE_H
#define SESSIONSERVICE_H

#include <QObject>
#include <QSettings>

class SessionService : public QObject
{
    Q_OBJECT

public:
    explicit SessionService(QObject *parent = nullptr);

    QString activeZowiDeviceAddress() const;
    QString activeZowiName() const;
    bool wizardDismissed() const;

    void setActiveZowiDeviceAddress(const QString &address);
    void setActiveZowiName(const QString &name);
    void setWizardDismissed(bool dismissed);

signals:
    void sessionChanged();

private:
    QSettings m_settings;
    QString sanitizeZowiName(const QString &name) const;
};

#endif
