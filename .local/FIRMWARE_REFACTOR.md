# Firmware flashing: shared orchestration plan (A/B)

Context: `Zowi::firmware` (`src/firmware/stk500v1.cpp`, `stk500UploadFirmware`)
is genuinely shared — both CLI and GUI call the same STK500v1 + Intel-HEX
engine with their own `BootloaderTransport`. The **orchestration around
flashing** (connection lifecycle, bootloader reset, upload-mode byte routing,
battery check, post-flash reconnect) is duplicated and has already drifted,
producing the USB "Failed to synchronize with the bootloader" bug (GUI called
`pulseReset()` before `connect()`, which is a no-op).

## Current duplication

| Step | CLI (`src/cli/`) | GUI (`src/gui/controllers/RobotController.cpp`) |
|---|---|---|
| Connection | `prepareFlashBackend` + `bt->connect()` (fresh) | reuses live `m_backend`: `disconnect` + `connect` |
| Bootloader reset | none explicit (relies on 1st-open auto-reset) | `serial->pulseReset()` — must be called AFTER `connect()` |
| Upload-mode byte routing | `g_uploadMode` + `g_stkBuffer` (`cli_util.cpp`) | `m_uploadMode` + `m_stkBuffer` (`RobotController.cpp`) |
| Battery check | `waitForBatteryLevel` | GUI signals + `confirmRestoreBattery` |
| Post-flash | `bt.disconnect()` | `continueAfterUpload` reopens at `usb_baud` + `connectUsb` |
| appId confirm | `waitForAppId` | not done by GUI |

## Decision (pending user choice)

- **A) Minimal shared helper**: e.g. `installFirmwareViaBackend(backend, path,
  options)`: upload-mode → stable connect → bootloader reset (pulseReset for
  serial) → `stk500UploadFirmware` → reconnect/post-checks. CLI and GUI call
  it; each keeps its battery/appId specifics via callbacks.
- **B) Full refactor**: also centralise upload-mode routing and battery in the
  common layer with callbacks; CLI/GUI only supply transport + signals.

Recommended: **A** now (CLI already works; centralise the critical sequence so
it cannot diverge again), **B** as follow-up.

## Minimal fix already applied (do this regardless)

In `RobotController::proceedWithRestore()` (USB path): call
`serial->pulseReset()` **after** `m_backend->connect()` succeeds (port open) so
the DTR falling edge reliably resets the MCU into the bootloader before the STK
sync. No structural change.