#include "BluetoothController.h"
#include "services/BluetoothService.h"
#include "core/DeviceInfo.h"

BluetoothController::BluetoothController(BluetoothService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
{
    connect(m_service, &BluetoothService::deviceDiscovered, this, [this](const DeviceInfo &d) {
        emit deviceDiscovered(d.name, d.address);
    });
    connect(m_service, &BluetoothService::scanFinished, this, &BluetoothController::scanFinished);
    connect(m_service, &BluetoothService::connectionChanged, this, &BluetoothController::connectionChanged);
    connect(m_service, &BluetoothService::scanningChanged, this, &BluetoothController::scanningChanged);
    connect(m_service, &BluetoothService::deviceChanged, this, &BluetoothController::deviceChanged);
    connect(m_service, &BluetoothService::dataReceived, this, &BluetoothController::dataReceived);
    connect(m_service, &BluetoothService::errorOccurred, this, &BluetoothController::errorOccurred);
}

bool BluetoothController::isConnected() const { return m_service->isConnected(); }
bool BluetoothController::isScanning() const { return m_service->isScanning(); }
QString BluetoothController::deviceName() const { return m_service->deviceName(); }
QString BluetoothController::deviceAddress() const { return m_service->deviceAddress(); }

void BluetoothController::startScan() { m_service->startScan(); }
void BluetoothController::stopScan() { m_service->stopScan(); }
void BluetoothController::connectToDevice(const QString &address) { m_service->connectToDevice(address); }
void BluetoothController::disconnectFromDevice() { m_service->disconnectFromDevice(); }
void BluetoothController::sendData(const QString &data) { m_service->sendData(data); }
