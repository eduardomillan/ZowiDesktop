# Transport Management (Bluetooth / USB) — How To

> Design document and test checklist for ZowiDesktop's transport abstraction layer.
> Covers Bluetooth SPP, USB serial, and the situation state machine that ties
> registration, connection, and transport selection together.

---

## Table of contents

- [1. Background & Design Decisions](#1-background--design-decisions)
- [2. Situation State Machine](#2-situation-state-machine)
- [3. State Resolution Logic](#3-state-resolution-logic)
- [4. Registration Flows](#4-registration-flows)
- [5. Rename & Firmware on Re-registration](#5-rename--firmware-on-re-registration)
- [6. Test Checklist (Casuistics)](#6-test-checklist-casuistics)
- [7. Persistence Keys (SessionStore)](#7-persistence-keys-sessionstore)
- [8. UI Integration](#8-ui-integration)
- [9. Build & Test](#9-build--test)
- [10. CLI Integration](#10-cli-integration)
- [11. Implementation Pointers](#11-implementation-pointers)
- [12. Adding a New Transport](#12-adding-a-new-transport)

---

## 1. Background & Design Decisions

ZowiDesktop originated from BQ's Android app (2015), which only supported
Bluetooth SPP. During desktop development, USB serial was added as an
alternative for machines without Bluetooth.

**Key design principles:**

1. **Transport is not a user preference** — it is a *consequence* of
   (available transports + registration + connection result). The code resolves
   the situation and offers only **contextual actions** (see state machine).

2. **Registration binds the transport** — `activeZowiTransport` (bt/usb) is
   persisted with the device address. Changing transport requires *forgetting*
   the Zowi; there is no live transport switch.

3. **Priority (no registration)** — If both Bt and USB are present and no
   Zowi is registered, **Bluetooth is preferred**. If the same robot appears on
   both, warn the user to unplug the USB cable.

4. **No pairing on USB** — USB registration is direct: detect the serial port,
   confirm connection, done.

5. **Forgot = app-side only** — "Forget Zowi" clears app data and unpairs at
   the OS level. It **never rewrites the robot's EEPROM** (name or firmware).

---

## 2. Situation State Machine

The `RobotController` computes a `situation` enum on every relevant event:

| Situation | Meaning |
|-----------|---------|
| `Demo` | No Bt/USB; headless/simulated mode |
| `Unregistered` | Transport(s) available, no Zowi registered |
| `Connecting` | Attempting to connect to registered device |
| `Connected` | Live link; robot operational |
| `Disconnected` | Registered but link down; auto-retry |
| `TransportLost` | Registered transport unavailable (e.g., Bt adapter gone) |

### State Diagram (simplified)

```
START
  │
  ├─ registered? ──no──► (btAvail || usbAvail)? ──no──► DEMO
  │                          │
  │                         yes
  │                          ▼
  │                 UNREGISTERED
  │                    (bt=pairing, usb=direct)
  │
  └─yes─► regTransport available? ──no──► TRANSPORT_LOST
       │
      yes
       ▼
   CONNECTING
     │
     ├─ connects? ──yes──► CONNECTED
     │
     └─no──► DISCONNECTED ──retry──► CONNECTING
```

### Contextual Actions by State

| State | Actions |
|-------|---------|
| `CONNECTED` | None (normal app actions) |
| `CONNECTING` | Cancel |
| `DISCONNECTED` | Retry; Forget Zowi; suggest other transport if available |
| `TRANSPORT_LOST` | Retry when transport returns; Forget & re-register on available transport; Demo |
| `UNREGISTERED` | Start registration (Bt=pairing / USB=direct) per availability |
| `DEMO` | On Bt/USB appear → exit demo → start registration |

### Invariant Rules

- **Registered transport is fixed** — changing it requires *Forget Zowi*.
- **Bt priority when both present without registration** — warn if same robot on both.
- **Hotplug awareness** — any change in `btAvail`/`usbAvail`/`connected` re-evaluates state and updates UI without app restart.

---

## 3. State Resolution Logic

The `RobotController` re-evaluates the situation on these signals:

- `btAvail` / `usbAvail` — transport availability (hotplug)
- `registered` — `activeZowiDeviceAddress` exists
- `regTransport` — `activeZowiTransport` ("bt" | "usb")
- `connected` — live link to robot

**Resolution order:**

1. If not registered → `Unregistered` (if any transport) or `Demo` (none)
2. If registered & `regTransport` available → `Connecting`
3. If registered & `regTransport` NOT available → `TransportLost`
4. While `Connecting`:
   - Success → `Connected`
   - Fail → `Disconnected` (auto-retry)
5. If `Connected` drops → `Disconnected` (auto-retry)

---

## 4. Registration Flows

### Bluetooth (pairing)

1. Scan → select device → pair (PIN 1234)
2. Connect → read name, `appId`, battery
4. Persist: `activeZowiDeviceAddress`, `activeZowiTransport="bt"`,
   `activeZowiName`, `activeZowiAppId`, `wizardDismissed=true`
5. If name ≠ default → show `WizardRenameScreen` (pre-filled)

### USB (direct)

1. Enumerate serial ports → user selects / auto-detects
2. Connect → read name, `appId`, battery
3. Persist same keys, `activeZowiTransport="usb"`

---

## 5. Rename & Firmware on Re-registration

When a previously renamed / re-flashed robot is registered again:

- **Name** — if robot reports name ≠ `zowi_default_name` (case-insensitive),
  `WizardRenameScreen` is **skipped** and name is kept. A `MessageBar` shows:
  *"Robot already named 'X'. Keeping it."*
- **Firmware (`appId`)** — read from `&&I <appId>%%`, persisted as
  `activeZowiAppId`. **Never restored** on re-registration.
- **Rule** — *Forget* only clears app data and OS pairing; never touches robot EEPROM.

---

## 6. Test Checklist (Casuistics)

> Format: `[ ]` = pending · `[x]` = verified

### A. Transport availability at start (no registration)

| ID | Scenario | Status |
|----|----------|--------|
| A1 | Only Bluetooth available | `[x]` |
| A2 | Only USB available | `[x]` |
| A3 | Both available (Bt priority, USB warning) | `[x]` |
| A4 | Neither available (Demo mode) | `[x]` |

### B. Previous registration

| ID | Scenario | Status |
|----|----------|--------|
| B1 | Not registered + Bt available | `[x]` |
| B2 | Not registered + USB only | `[x]` |
| B3 | Registered by Bt (USB discarded) | `[x]` |
| B4 | Registered by USB (Bt discarded) | `[x]` |
| B5 | Registered transport unavailable | `[x]` |

### C. Connection result

| ID | Scenario | Status |
|----|----------|--------|
| C1 | Registered + connects | `[x]` |
| C2 | Registered + fails (offline/range/cable) | `[x]` |
| C3 | Not registered + connects during wizard | `[x]` |
| C4 | Not registered + fails during wizard | `[x]` |
| C5 | Hot loss of connection | `[x]` |

### D. Edge cases & conflicts

| ID | Scenario | Status |
|----|----------|--------|
| D1 | Same robot on Bt + USB simultaneously | `[x]` |
| D2 | Different robot on USB vs registered Bt | `[x]` |
| D3 | Hotplug transport appear/disappear | `[x]` |
| D4 | Demo → transport appears | `[x]` |

### E. Re-registration of renamed / re-flashed robot

| ID | Scenario | Status |
|----|----------|--------|
| E2 | Name ≠ default → skip rename, show notice | `[x]` |
| E3 | MessageBar shows existing name | `[x]` |
| E4 | `appId` parsed & persisted | `[x]` |
| E5 | `appId` shown in StatusBar + DevOverlay | `[x]` |

---

## 7. Persistence Keys (SessionStore)

Both GUI and CLI share these keys (namespace `ZowiDesktop` / `ZowiApp`):

| Key | Type | Meaning |
|-----|------|---------|
| `activeZowiDeviceAddress` | string | Bt MAC or USB port identifier |
| `activeZowiName` | string | User-visible name |
| `activeZowiAppId` | string | Firmware identifier from `&&I` |
| `activeZowiTransport` | string | `"bt"` or `"usb"` (bound to registration) |
| `activeZowiBattery` | int | Last known battery % |
| `wizardDismissed` | bool | Skip welcome wizard |

**Config directory resolution** — platform default or explicit `configDir`:

- Linux: `$XDG_CONFIG_HOME` / `~/.config` + `/ZowiDesktop/ZowiApp.json`
- macOS: `~/Library/Application Support/ZowiDesktop/ZowiApp.json`
- Windows: `%APPDATA%\ZowiDesktop\ZowiApp.json`

---

## 8. UI Integration

- **SettingsScreen** — shows current `situation` + contextual actions
  (retry, forget, connect USB, register). No transport picker.
- **StatusBar** — shows connection state, battery, active transport pill,
  firmware pill (`FW <appId>`).
- **DevOverlay** — shows `Firmware (appId)` line.
- **WizardRenameScreen** — auto-skipped if robot name ≠ default.

---

## 9. Build & Test

```bash
# Build GUI (includes transport backends)
./build.sh --gui

# Run headless smoke test
QT_QPA_PLATFORM=offscreen ./build/src/gui/ZowiDesktop

# Core unit tests (includes transport constants)
ctest --test-dir build --output-on-failure
```

---

## 9. CLI Integration

The CLI (`zowi_cli`) uses the same `SessionStore` and transport backends.
Commands that flash firmware (`restore`, `alarm`, `adivinawi`) respect the
registered transport and `activeZowiDeviceAddress`.

```bash
zowi_cli status        # Shows situation, transport, battery
zowi_cli scan          # Bt scan
zowi_cli connect <MAC> # Bt connect
zowi_cli ports         # USB serial ports
zowi_cli restore       # Flashes over registered transport
```

---

## 10. Implementation Pointers

| Component | File |
|-----------|------|
| Situation enum & state machine | `src/core/include/zowi/robot_state.h` / `.cpp` |
| RobotController (state resolution) | `src/gui/controllers/RobotController.h` / `.cpp` |
| Bluetooth backend (Qt/BlueZ) | `src/backends/bt_qt/` |
| USB serial backend (POSIX) | `src/backends/bt_serial/` |
| USB serial backend (Win32) | `src/backends/bt_serial_win/` |
| Native Windows Bt (WinRT) | `src/backends/bt_native/` |
| Session persistence | `src/core/include/zowi/session_store.h` / `.cpp` |
| Settings UI | `src/views/screens/SettingsScreen.qml` |
| StatusBar | `src/views/components/StatusBar.qml` |

---

## 11. Adding a New Transport

To add a transport (e.g., Wi-Fi, BLE):

1. Implement `BluetoothApi` in `src/backends/<new>/`
2. Add to `src/backends/CMakeLists.txt` and GUI target link libraries
3. Extend `RobotController::Transport` enum (`TransportWifi`, etc.)
4. Update `RobotController::refreshTransports()` to detect availability
5. Update situation logic for new transport priority
6. Add transport-specific UI strings to i18n files

---

*End of document*