#include "BluetoothNativeLoader.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
// Callback trampolines for Win32 DLL calls
static void WINAPI discovery_trampoline(const char *addr, const char *name, int rssi, void *user_data) {
    Q_UNUSED(addr); Q_UNUSED(name); Q_UNUSED(rssi); Q_UNUSED(user_data);
}

static void WINAPI connection_trampoline(int connected, void *user_data) {
    auto *loader = static_cast<BluetoothNativeLoader *>(user_data);
    if (connected)
        emit loader->deviceConnected();
    else
        emit loader->deviceDisconnected();
}

static void WINAPI data_trampoline(const char *data, int length, void *user_data) {
    auto *loader = static_cast<BluetoothNativeLoader *>(user_data);
    emit loader->dataReceived(QByteArray(data, length));
}
#endif

BluetoothNativeLoader::BluetoothNativeLoader(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
    loadDll();
#else
    m_error = "Native Bluetooth DLL only available on Windows";
#endif
}

BluetoothNativeLoader::~BluetoothNativeLoader()
{
    cleanup();
}

bool BluetoothNativeLoader::loadDll()
{
#ifdef Q_OS_WIN
    // Search in exe directory first, then system
    QString dllName = "bt_native.dll";
    QString exeDir = QCoreApplication::applicationDirPath();
    QString dllPath = exeDir + "/" + dllName;

    m_hModule = LoadLibraryW(dllPath.toStdWString().c_str());
    if (!m_hModule) {
        m_error = QString("Could not load %1 (error %2)")
                      .arg(dllName).arg(GetLastError());
        qWarning() << "[bt_native]" << m_error;
        return false;
    }

    // Resolve function pointers
    p_bt_init = reinterpret_cast<PFN_bt_init>(GetProcAddress(m_hModule, "bt_init"));
    p_bt_start_discovery = reinterpret_cast<PFN_bt_start_discovery>(GetProcAddress(m_hModule, "bt_start_discovery"));
    p_bt_stop_discovery = reinterpret_cast<PFN_bt_stop_discovery>(GetProcAddress(m_hModule, "bt_stop_discovery"));
    p_bt_connect = reinterpret_cast<PFN_bt_connect>(GetProcAddress(m_hModule, "bt_connect"));
    p_bt_send = reinterpret_cast<PFN_bt_send>(GetProcAddress(m_hModule, "bt_send"));
    p_bt_start_receive = reinterpret_cast<PFN_bt_start_receive>(GetProcAddress(m_hModule, "bt_start_receive"));
    p_bt_disconnect = reinterpret_cast<PFN_bt_disconnect>(GetProcAddress(m_hModule, "bt_disconnect"));
    p_bt_cleanup = reinterpret_cast<PFN_bt_cleanup>(GetProcAddress(m_hModule, "bt_cleanup"));
    p_bt_last_error = reinterpret_cast<PFN_bt_last_error>(GetProcAddress(m_hModule, "bt_last_error"));

    if (!p_bt_init || !p_bt_connect || !p_bt_send || !p_bt_disconnect) {
        m_error = "bt_native.dll missing required exports";
        qWarning() << "[bt_native]" << m_error;
        unloadDll();
        return false;
    }

    m_available = true;
    qInfo() << "[bt_native] Loaded successfully from" << dllPath;
    return true;
#else
    m_error = "Not supported on this platform";
    return false;
#endif
}

void BluetoothNativeLoader::unloadDll()
{
#ifdef Q_OS_WIN
    if (m_hModule) {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }
    m_available = false;
    p_bt_init = nullptr;
    p_bt_start_discovery = nullptr;
    p_bt_stop_discovery = nullptr;
    p_bt_connect = nullptr;
    p_bt_send = nullptr;
    p_bt_start_receive = nullptr;
    p_bt_disconnect = nullptr;
    p_bt_cleanup = nullptr;
    p_bt_last_error = nullptr;
#endif
}

bool BluetoothNativeLoader::init()
{
    if (!m_available) return false;
#ifdef Q_OS_WIN
    int result = p_bt_init();
    if (result != 0) {
        m_error = QString("bt_init failed: %1")
                      .arg(p_bt_last_error ? p_bt_last_error() : "unknown");
        emit errorOccurred(m_error);
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool BluetoothNativeLoader::connectDevice(const QString &address)
{
    if (!m_available) return false;
#ifdef Q_OS_WIN
    QByteArray addr = address.toUtf8();
    int result = p_bt_connect(addr.constData(), reinterpret_cast<void *>(connection_trampoline), this);
    if (result != 0) {
        m_error = QString("bt_connect failed: %1")
                      .arg(p_bt_last_error ? p_bt_last_error() : "unknown");
        emit errorOccurred(m_error);
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool BluetoothNativeLoader::sendData(const QByteArray &data)
{
    if (!m_available) return false;
#ifdef Q_OS_WIN
    int sent = p_bt_send(data.constData(), data.size());
    if (sent < 0) {
        m_error = QString("bt_send failed: %1")
                      .arg(p_bt_last_error ? p_bt_last_error() : "unknown");
        emit errorOccurred(m_error);
        return false;
    }
    return true;
#else
    return false;
#endif
}

void BluetoothNativeLoader::disconnectDevice()
{
    if (!m_available) return;
#ifdef Q_OS_WIN
    p_bt_disconnect();
#endif
}

void BluetoothNativeLoader::cleanup()
{
    if (!m_available) return;
#ifdef Q_OS_WIN
    p_bt_cleanup();
    unloadDll();
#endif
}
