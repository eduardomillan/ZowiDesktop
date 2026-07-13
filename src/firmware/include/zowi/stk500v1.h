#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>

namespace zowi {

// Transport abstraction used to talk to the device's bootloader. The host side
// (CLI / GUI) provides the actual byte I/O; this module implements the STK500v1
// / Optiboot protocol on top of it.
struct BootloaderTransport {
    // Send raw bytes to the device. Returns true on success.
    std::function<bool(const std::vector<uint8_t> &)> send;

    // Fill `out` with up to `maxBytes` bytes currently available from the device.
    // Returns the number of bytes placed in `out` (0 if none available yet), or
    // -1 on a transport error.
    std::function<int(std::vector<uint8_t> &out, std::size_t maxBytes)> receive;

    // Pump the host's event loop so asynchronous I/O (e.g. Qt signals) can run
    // while the protocol blocks waiting for a reply.
    std::function<void()> pump;
};

// Upload a firmware image given as an Intel HEX file to the device using the
// STK500v1 / Optiboot bootloader protocol over the provided transport.
//
// Flow: soft-reset into bootloader -> STK_GET_SYNC -> STK_ENTER_PROGMODE ->
// for each 128-byte flash page { STK_LOAD_ADDRESS, STK_PROG_PAGE } ->
// STK_LEAVE_PROGMODE -> soft-reset into the new firmware.
//
// Returns true when the firmware was written successfully.
bool stk500UploadFirmware(const BootloaderTransport &transport,
                          const std::string &hexPath);

// Upload a firmware image by streaming the raw HEX file to the device's
// bootloader. Some Zowi firmwares expose a custom bootloader that expects the
// raw Intel HEX bytes (the same stream the original Android app sends) rather
// than the STK500v1 framing. The soft-reset sequence is sent first to make the
// running firmware reboot into its bootloader.
//
// Returns true when the stream was sent successfully.
bool zowiRawHexUploadFirmware(const BootloaderTransport &transport,
                              const std::string &hexPath);

} // namespace zowi
