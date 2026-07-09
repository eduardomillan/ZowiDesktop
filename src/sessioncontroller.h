#ifndef SESSIONCONTROLLER_H
#define SESSIONCONTROLLER_H

#include <QObject>
#include <QSettings>

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

signals:
    void sessionChanged();

private:
    QSettings m_settings;
    QString sanitizeZowiName(const QString &name) const;
};

#endif
