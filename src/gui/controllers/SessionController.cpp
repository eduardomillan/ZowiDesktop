#include "SessionController.h"

SessionController::SessionController(QObject *parent)
    : QObject(parent)
    , m_store("ZowiDesktop", "ZowiApp")
{
    m_store.onChanged([this]() { emit sessionChanged(); });
}

QString SessionController::loadActiveZowiDeviceAddress()
{
    return QString::fromStdString(m_store.getString("activeZowiDeviceAddress"));
}

QString SessionController::loadActiveZowiName()
{
    return QString::fromStdString(m_store.getString("activeZowiName", "Zowi"));
}

bool SessionController::hasDismissedWizard()
{
    return m_store.getBool("wizardDismissed");
}

void SessionController::saveActiveZowiDeviceAddress(const QString &address)
{
    m_store.setString("activeZowiDeviceAddress", address.toStdString());
}

void SessionController::saveActiveZowiName(const QString &name)
{
    std::string sanitizedName = name.trimmed().toStdString();
    if (sanitizedName.empty() || sanitizedName == "true" || sanitizedName == "false"
        || sanitizedName == "True" || sanitizedName == "False") {
        sanitizedName = "Zowi";
    }
    m_store.setString("activeZowiName", sanitizedName);
}

void SessionController::saveWizardDismissed(bool dismissed)
{
    m_store.setBool("wizardDismissed", dismissed);
}

QStringList SessionController::keys() const
{
    QStringList result;
    for (const auto &key : m_store.keys())
        result.append(QString::fromStdString(key));
    return result;
}

QString SessionController::getRaw(const QString &key) const
{
    return QString::fromStdString(m_store.getRaw(key.toStdString()));
}

void SessionController::clearActive()
{
    for (const auto &key : m_store.keys()) {
        if (key.rfind("active", 0) == 0)
            m_store.removeKey(key);
    }
}
