#include "win_serial_backend.h"

#include <SetupAPI.h>
#include <devguid.h>
#include <regex>

#pragma comment(lib, "setupapi.lib")

namespace zowi {

namespace {

// Map a numeric baud rate to a DCB BaudRate value.
int baudToDcbBaud(int baud)
{
    switch (baud) {
        case 1200:   return CBR_1200;
        case 2400:   return CBR_2400;
        case 4800:   return CBR_4800;
        case 9600:   return CBR_9600;
        case 19200:  return CBR_19200;
        case 38400:  return CBR_38400;
        case 57600:  return CBR_57600;
        case 115200: return CBR_115200;
        case 230400: return 230400;
        default:     return CBR_9600;
    }
}

// Probe COM ports by attempting to open each one. This is more reliable than
// reading the registry because USB-serial adapters appear/disappear dynamically.
std::vector<std::string> probeComPorts()
{
    std::vector<std::string> ports;
    wchar_t portName[32];

    for (int i = 1; i <= 255; ++i) {
        _snwprintf_s(portName, _countof(portName), L"COM%d", i);
        std::wstring devicePath = std::wstring(L"\\\\.\\") + portName;

        HANDLE h = CreateFileW(
            devicePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            CloseHandle(h);
            // Convert wide string to narrow for the port name (e.g. "COM3").
            char narrow[32];
            WideCharToMultiByte(CP_UTF8, 0, portName, -1, narrow, sizeof(narrow), nullptr, nullptr);
            ports.push_back(narrow);
        }
    }

    return ports;
}

} // namespace

WinSerialBackend::WinSerialBackend(QObject *parent)
    : QObject(parent)
{
}

WinSerialBackend::~WinSerialBackend()
{
    disconnect();
}

bool WinSerialBackend::connect(const std::string &portName)
{
    disconnect();

    // Build the device path: "COM3" -> "\\\\.\\COM3"
    std::string devicePath = "\\\\.\\" + portName;

    m_handle = CreateFileA(
        devicePath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);

    if (m_handle == INVALID_HANDLE_VALUE) {
        m_lastError = "Failed to open " + portName + " (error " +
                      std::to_string(GetLastError()) + ")";
        return false;
    }

    // Configure the port buffer sizes.
    SetupComm(m_handle, 4096, 4096);

    // Configure DCB: 8N1, no flow control, requested baud rate.
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(m_handle, &dcb)) {
        m_lastError = "GetCommState failed";
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    dcb.BaudRate = baudToDcbBaud(m_baud);
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary  = TRUE;
    dcb.fParity  = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl   = DTR_CONTROL_ENABLE;
    dcb.fRtsControl   = RTS_CONTROL_ENABLE;
    dcb.fOutX   = FALSE;
    dcb.fInX    = FALSE;

    if (!SetCommState(m_handle, &dcb)) {
        m_lastError = "SetCommState failed";
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Set timeouts: read returns after 50ms even if no data (non-blocking feel).
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout         = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier  = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant   = 5000;
    SetCommTimeouts(m_handle, &timeouts);

    // Flush any stale data.
    PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    // The DTR reset pulse is intentionally NOT performed here. A normal
    // control connection must not reboot the robot. Firmware flashing calls
    // pulseReset() explicitly before connect().
    //
    // Settle so the firmware UART is ready. On a USB-serial link the MCU
    // resets when the port is opened and the bootloader runs for a moment
    // before handing control to the running firmware.
    if (m_bootDelayMs > 0)
        Sleep(static_cast<DWORD>(m_bootDelayMs));

    // Start the reader thread.
    m_reading = true;
    m_readThread = std::thread(&WinSerialBackend::readLoop, this);

    if (m_onConnect) m_onConnect(true);
    return true;
}

void WinSerialBackend::disconnect()
{
    // Stop the reader thread first.
    m_reading = false;

    if (m_readThread.joinable()) {
        m_readThread.join();
    }

    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }

    if (m_onConnect) m_onConnect(false);
}

bool WinSerialBackend::send(const std::string &data)
{
    if (m_handle == INVALID_HANDLE_VALUE) return false;

    const char *p = data.data();
    std::size_t remaining = data.size();
    while (remaining > 0) {
        DWORD written = 0;
        if (!WriteFile(m_handle, p, static_cast<DWORD>(remaining), &written, nullptr)) {
            m_lastError = "WriteFile failed (error " + std::to_string(GetLastError()) + ")";
            return false;
        }
        p += written;
        remaining -= static_cast<std::size_t>(written);
    }
    return true;
}

void WinSerialBackend::pulseReset()
{
    if (m_handle == INVALID_HANDLE_VALUE) return;

    // Ensure DTR starts high, then drive it low (falling edge resets MCU
    // through the coupling capacitor), then back high.
    EscapeCommFunction(m_handle, SETDTR);
    Sleep(10);
    EscapeCommFunction(m_handle, CLRDTR);
    Sleep(50);
    EscapeCommFunction(m_handle, SETDTR);
    Sleep(10);
}

std::vector<std::string> WinSerialBackend::listSerialPorts()
{
    return probeComPorts();
}

void WinSerialBackend::readLoop()
{
    char buf[512];
    while (m_reading.load()) {
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(m_handle, buf, sizeof(buf), &bytesRead, nullptr);

        if (!ok) {
            if (!m_reading.load()) break;
            // Transient error (e.g. timeout) — keep reading.
            continue;
        }

        if (bytesRead > 0 && m_onData) {
            std::string data(buf, static_cast<std::size_t>(bytesRead));
            // Marshal to the Qt thread for safe callback invocation.
            QMetaObject::invokeMethod(this, [this, data]() {
                if (m_onData) m_onData(data);
            }, Qt::QueuedConnection);
        }
    }
}

} // namespace zowi
