#include "zowi/firmware_installer.h"

#include <algorithm>
#include <iostream>

namespace zowi {

void FirmwareInstaller::enable()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = true;
    m_buffer.clear();
}

void FirmwareInstaller::disable()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = false;
}

bool FirmwareInstaller::isEnabled() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enabled;
}

void FirmwareInstaller::feed(const std::string &data)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_enabled) m_buffer += data;
}

int FirmwareInstaller::readBuffer(std::vector<uint8_t> &out, std::size_t maxBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_buffer.empty()) return 0;
    const std::size_t n = std::min(m_buffer.size(), maxBytes);
    out.assign(m_buffer.data(), m_buffer.data() + n);
    m_buffer.erase(0, n);
    return static_cast<int>(n);
}

bool FirmwareInstaller::upload(
    const std::string &hexPath,
    const std::string &protocol,
    std::function<bool(const std::vector<uint8_t> &)> sendFn,
    std::function<void()> pumpFn,
    std::function<void(int, std::size_t, std::size_t)> progressFn)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_enabled) {
            m_enabled = true;
            m_buffer.clear();
        }
    }

    BootloaderTransport transport;
    transport.send = std::move(sendFn);
    transport.receive = [this](std::vector<uint8_t> &out, std::size_t maxBytes) -> int {
        return readBuffer(out, maxBytes);
    };
    transport.pump = std::move(pumpFn);
    transport.progress = std::move(progressFn);

    const bool ok = (protocol == "stk")
                        ? stk500UploadFirmware(transport, hexPath)
                        : zowiRawHexUploadFirmware(transport, hexPath);

    disable();
    return ok;
}

void FirmwareInstaller::clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enabled = false;
    m_buffer.clear();
}

} // namespace zowi
