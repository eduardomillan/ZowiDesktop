#ifndef BLUETOOTHNATIVELOADER_H
#define BLUETOOTHNATIVELOADER_H

#include <QObject>
#include <QString>
#include <QByteArray>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// C function pointer types matching bt_native.h
typedef int (*PFN_bt_init)(void);
typedef int (*PFN_bt_start_discovery)(void *callback, void *user_data);
typedef void (*PFN_bt_stop_discovery)(void);
typedef int (*PFN_bt_connect)(const char *address, void *callback, void *user_data);
typedef int (*PFN_bt_send)(const char *data, int length);
typedef int (*PFN_bt_start_receive)(void *callback, void *user_data);
typedef void (*PFN_bt_disconnect)(void);
typedef void (*PFN_bt_cleanup)(void);
typedef const char *(*PFN_bt_last_error)(void);

class BluetoothNativeLoader : public QObject
{
    Q_OBJECT

public:
    explicit BluetoothNativeLoader(QObject *parent = nullptr);
    ~BluetoothNativeLoader();

    bool isAvailable() const { return m_available; }
    QString lastError() const { return m_error; }

    // High-level API
    bool init();
    bool connectDevice(const QString &address);
    bool sendData(const QByteArray &data);
    void disconnectDevice();
    void cleanup();

signals:
    void deviceConnected();
    void deviceDisconnected();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &message);

private:
    bool loadDll();
    void unloadDll();

    bool m_available = false;
    QString m_error;

#ifdef Q_OS_WIN
    HMODULE m_hModule = nullptr;
#endif

    // Function pointers
    PFN_bt_init p_bt_init = nullptr;
    PFN_bt_start_discovery p_bt_start_discovery = nullptr;
    PFN_bt_stop_discovery p_bt_stop_discovery = nullptr;
    PFN_bt_connect p_bt_connect = nullptr;
    PFN_bt_send p_bt_send = nullptr;
    PFN_bt_start_receive p_bt_start_receive = nullptr;
    PFN_bt_disconnect p_bt_disconnect = nullptr;
    PFN_bt_cleanup p_bt_cleanup = nullptr;
    PFN_bt_last_error p_bt_last_error = nullptr;
};

#endif // BLUETOOTHNATIVELOADER_H
