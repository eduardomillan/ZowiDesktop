#include "SessionController.h"
#include "services/SessionService.h"

SessionController::SessionController(SessionService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    connect(m_service, &SessionService::sessionChanged,
            this, &SessionController::sessionChanged);
}

QString SessionController::loadActiveZowiDeviceAddress()
{
    return m_service->activeZowiDeviceAddress();
}

QString SessionController::loadActiveZowiName()
{
    return m_service->activeZowiName();
}

bool SessionController::hasDismissedWizard()
{
    return m_service->wizardDismissed();
}

void SessionController::saveActiveZowiDeviceAddress(const QString &address)
{
    m_service->setActiveZowiDeviceAddress(address);
}

void SessionController::saveActiveZowiName(const QString &name)
{
    m_service->setActiveZowiName(name);
}

void SessionController::saveWizardDismissed(bool dismissed)
{
    m_service->setWizardDismissed(dismissed);
}
