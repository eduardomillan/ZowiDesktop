#include "SessionService.h"

SessionService::SessionService(QObject *parent)
    : QObject(parent)
    , m_settings("ZowiDesktop", "ZowiApp")
{
}

QString SessionService::activeZowiDeviceAddress() const
{
    return m_settings.value("activeZowiDeviceAddress").toString();
}

QString SessionService::activeZowiName() const
{
    return sanitizeZowiName(m_settings.value("activeZowiName", "Zowi").toString());
}

bool SessionService::wizardDismissed() const
{
    return m_settings.value("wizardDismissed", false).toBool();
}

void SessionService::setActiveZowiDeviceAddress(const QString &address)
{
    m_settings.setValue("activeZowiDeviceAddress", address);
    emit sessionChanged();
}

void SessionService::setActiveZowiName(const QString &name)
{
    m_settings.setValue("activeZowiName", sanitizeZowiName(name));
    emit sessionChanged();
}

void SessionService::setWizardDismissed(bool dismissed)
{
    m_settings.setValue("wizardDismissed", dismissed);
    emit sessionChanged();
}

QString SessionService::sanitizeZowiName(const QString &name) const
{
    QString normalized = name.trimmed();
    if (normalized.isEmpty()
        || normalized.compare("false", Qt::CaseInsensitive) == 0
        || normalized.compare("true", Qt::CaseInsensitive) == 0) {
        return "Zowi";
    }
    return normalized;
}
