#include "sessioncontroller.h"

SessionController::SessionController(QObject *parent)
    : QObject(parent)
    , m_settings("ZowiDesktop", "ZowiApp")
{
}

QString SessionController::loadActiveZowiDeviceAddress()
{
    return m_settings.value("activeZowiDeviceAddress").toString();
}

QString SessionController::loadActiveZowiName()
{
    return sanitizeZowiName(m_settings.value("activeZowiName", "Zowi").toString());
}

bool SessionController::hasDismissedWizard()
{
    return m_settings.value("wizardDismissed", false).toBool();
}

void SessionController::saveActiveZowiDeviceAddress(const QString &address)
{
    m_settings.setValue("activeZowiDeviceAddress", address);
    emit sessionChanged();
}

void SessionController::saveActiveZowiName(const QString &name)
{
    m_settings.setValue("activeZowiName", sanitizeZowiName(name));
    emit sessionChanged();
}

void SessionController::saveWizardDismissed(bool dismissed)
{
    m_settings.setValue("wizardDismissed", dismissed);
    emit sessionChanged();
}

QString SessionController::sanitizeZowiName(const QString &name) const
{
    QString normalized = name.trimmed();
    if (normalized.isEmpty()
        || normalized.compare("false", Qt::CaseInsensitive) == 0
        || normalized.compare("true", Qt::CaseInsensitive) == 0) {
        return "Zowi";
    }
    return normalized;
}
