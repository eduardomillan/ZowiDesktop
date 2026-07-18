#pragma once

#include <QObject>
#include <zowi/session_store.h>

class SessionController : public QObject
{
    Q_OBJECT

public:
    explicit SessionController(QObject *parent = nullptr);

    Q_INVOKABLE QString loadActiveZowiDeviceAddress();
    Q_INVOKABLE QString loadActiveZowiName();
    Q_INVOKABLE bool hasDismissedWizard();

    Q_INVOKABLE void saveActiveZowiDeviceAddress(const QString &address);
    Q_INVOKABLE void saveActiveZowiName(const QString &name);
    Q_INVOKABLE void saveWizardDismissed(bool dismissed);

    Q_INVOKABLE QStringList keys() const;
    Q_INVOKABLE QString getRaw(const QString &key) const;
    Q_INVOKABLE void clearActive();

    // Generic key access for non-Zowi-specific session values (e.g. "transport").
    Q_INVOKABLE void saveString(const QString &key, const QString &value);
    Q_INVOKABLE QString getString(const QString &key, const QString &defaultValue = QString()) const;

signals:
    void sessionChanged();


private:
    zowi::SessionStore m_store;
};
