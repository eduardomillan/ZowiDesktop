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

    // Transport with which the active Zowi was registered ("bluetooth" | "usb").
    // Empty when no Zowi is registered. Used to tie the transport to the
    // registration (changing transport requires forgetting the Zowi).
    Q_INVOKABLE QString loadActiveZowiTransport();
    Q_INVOKABLE void saveActiveZowiTransport(const QString &transport);

    // Firmware / program id reported by the robot (e.g. "&&I <appId>%%").
    // Persisted so the app remembers which firmware the connected Zowi runs.
    Q_INVOKABLE QString loadActiveZowiAppId();
    Q_INVOKABLE void saveActiveZowiAppId(const QString &appId);

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
