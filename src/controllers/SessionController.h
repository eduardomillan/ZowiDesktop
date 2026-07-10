#ifndef SESSIONCONTROLLER_H
#define SESSIONCONTROLLER_H

#include <QObject>

class SessionService;

class SessionController : public QObject
{
    Q_OBJECT

public:
    explicit SessionController(SessionService *service, QObject *parent = nullptr);

    Q_INVOKABLE QString loadActiveZowiDeviceAddress();
    Q_INVOKABLE QString loadActiveZowiName();
    Q_INVOKABLE bool hasDismissedWizard();

    Q_INVOKABLE void saveActiveZowiDeviceAddress(const QString &address);
    Q_INVOKABLE void saveActiveZowiName(const QString &name);
    Q_INVOKABLE void saveWizardDismissed(bool dismissed);

signals:
    void sessionChanged();

private:
    SessionService *m_service;
};

#endif
