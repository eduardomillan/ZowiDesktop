#pragma once

#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <zowi/stk500v1.h>

namespace zowi {

// Centralised firmware upload orchestrator.
//
// Manages the upload-mode buffer that routes incoming transport bytes away from
// the normal protocol parser and into a raw buffer consumed by the STK500v1 /
// Optiboot uploader. Both the CLI and the GUI (and future web / Android hosts)
// use this single class instead of reimplementing the buffer + transport wiring
// independently.
//
// Typical usage:
//   1.  FirmwareInstaller installer;
//   2.  installer.enable();                         // before the reset cycle
//   3.  ... stable connection established ...
//   4.  installer.upload(path, "stk", sendFn, pumpFn, progressFn);
//       (enable + clear + upload + disable happen inside upload())
//   5.  In the onDataReceived callback:  if (installer.isEnabled()) installer.feed(data);
//
// Thread safety: all public methods are safe to call from any thread. The
// internal buffer is guarded by a mutex.
class FirmwareInstaller {
public:
    // Enable upload mode. While enabled, feed() should be called from the
    // transport's onDataReceived callback so incoming bytes are captured in the
    // upload buffer instead of being parsed as protocol messages.
    void enable();

    // Disable upload mode. After this call, feed() is a no-op.
    void disable();

    // True when upload mode is active.
    bool isEnabled() const;

    // Feed raw bytes from the transport into the upload buffer.
    // No-op when upload mode is disabled.
    void feed(const std::string &data);

    // Run a firmware upload. The caller must have already prepared the
    // transport for bootloader mode (disconnect / reconnect / pulseReset / baud
    // switch — all of which are inherently host-specific).
    //
    // Internally this method:
    //   1. Enables upload mode and clears the buffer.
    //   2. Builds a BootloaderTransport wired to sendFn / the internal buffer.
    //   3. Calls stk500UploadFirmware() or zowiRawHexUploadFirmware().
    //   4. Disables upload mode.
    //
    // sendFn   — sends raw bytes to the device (wraps backend->send()).
    // pumpFn   — pumps the host event loop (Qt processEvents / CLI processEvents).
    // progressFn — optional progress callback (percent, written, total).
    //
    // Returns true when the firmware was written successfully.
    bool upload(const std::string &hexPath,
                const std::string &protocol,
                std::function<bool(const std::vector<uint8_t> &)> sendFn,
                std::function<void()> pumpFn,
                std::function<void(int, std::size_t, std::size_t)> progressFn = nullptr);

    // Reset the buffer and disable upload mode.
    void clear();

private:
    int readBuffer(std::vector<uint8_t> &out, std::size_t maxBytes);

    mutable std::mutex m_mutex;
    bool m_enabled = false;
    std::string m_buffer;
};

} // namespace zowi
