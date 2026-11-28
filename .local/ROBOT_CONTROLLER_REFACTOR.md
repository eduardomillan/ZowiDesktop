# RobotController refactor — move its logic into core

> Status: **plan, nothing implemented.** Decision taken: split the logic of
> `RobotController` into several Qt-free classes in `src/core`, and keep **one**
> `Robot` object (a thin facade) for QML.

## Why

- `src/gui/controllers/RobotController.cpp` (~1,430 lines) holds most of the
  connection logic of the app, and it is Qt/C++ code outside `src/core`. The rule of
  the project is that the GUI is an adapter layer, not a second business-logic layer.
- That logic can only be exercised today with a real robot. In core it can be tested
  with a fake `BluetoothApi` and a virtual clock.
- A future web frontend (WebAssembly) needs the same logic. Rewriting it in
  TypeScript would duplicate it; moving it to core lets desktop, CLI and web share
  one copy.
- The other controllers are thin adapters over core modules and do not need this
  work. Two are special: `OnlineRankingClient` stays in the host on purpose (core has
  no network code), and `TimelineController` (422 lines) should be reviewed, but
  core already has `timeline_player`, so it is probably thin.

## What `RobotController` does today

Three responsibilities, mixed in one class:

1. **Link and robot state** — connect, disconnect, `sendData`, identity polling
   (`requestRobotData` + the poll timer), parsing what the robot sends
   (`parseIncoming`), and the name, appId and battery state, including the
   low-battery rule (`checkLowBattery`, one threshold: `low_battery_threshold`).
2. **Transports and situation** — detecting Bluetooth/USB availability
   (`pollTransports`, `refreshTransports`), USB port probing and the identity check
   (`startUsbProbe`, `probeNextUsbPort`, `usbProbeTick`, `finishUsbProbe`), transport
   preference and switching (`setTransportPreference`, `switchTransport`,
   `revertTransport`), the connect-attempt timeout (`onConnectTimeout`), the
   registration/transport binding (`persistRegistrationTransport`) and the
   situation state machine (`computeSituation`: Demo / Unregistered / Connecting /
   Connected / Disconnected / TransportLost).
3. **Firmware restore** — `restoreFirmware`, the battery confirmation
   (`confirmRestoreBattery`), the upload, the reconnection and the finish
   (`proceedWithRestore`, `continueAfterUpload`, `finishRestore`). Part of it is
   already encapsulated in `m_firmwareInstaller`.

## Target design

### In core (Qt-free, `src/core`)

Three classes, one file each, working names:

| Class | Owns |
|---|---|
| `RobotLink` (or `RobotSession`) | The `BluetoothApi` connection, connect/disconnect/send, identity polling cadence, message application (uses `MessageParser` and `RobotState`), name/appId/battery, low-battery rule and its "warn once" state |
| `TransportManager` | Availability of Bluetooth and USB, USB probing and identity check, transport preference and switching, connect timeout, registration/transport binding, and the situation state machine |
| `FirmwareRestore` | The restore sequence: battery check and confirmation, upload through the injected bootloader transport, reconnection, result |

Rules:

- **Timers and the clock are injected** through a small interface (schedule a
  callback after N ms, cancel it, read the time). The Qt side implements it with
  `QTimer`; tests use a virtual clock that they advance by hand.
- **Anything that talks to Qt or the operating system stays outside**: serial-port
  enumeration, Bluetooth adapter queries, temporary files, clipboard. They are
  reached through small injected interfaces.
- `computeSituation` becomes a **pure function** of its inputs (registered address,
  registered transport, availability, connected, connecting, timed out).
- Session persistence goes through `SessionStore` (already in core).
- No behaviour changes: the refactor must be invisible to the user.

### On the Qt side

- **One `Robot` object for QML**, as now. About 20 screens use `Robot.*`, and the
  three parts share state (connected, backend, situation). Splitting them into three
  QML objects would force touching every screen and keeping signals in sync between
  them, which is the easiest way to introduce regressions.
- `RobotController` becomes a **facade**: it keeps the same properties, signals and
  invokable methods, owns the Qt timers and the Qt backends, and delegates to the
  three core classes.
- The restore flow could later be exposed as its own object (`Firmware`), since only
  `SettingsScreen` uses it. Optional and later.

## Steps

Small steps; each one must build, pass `ctest` and leave the app working.

1. **Pure pieces first** (about one session): the situation function, the
   low-battery rule and the connect timeout, with their core tests. The Qt
   controller calls them.
2. **Link and identity**: polling cadence, message application and the
   "warn once" low-battery state move to `RobotLink`.
3. **Transports and USB**: availability tracking, USB probe sequencing, identity
   check, preference and switching move to `TransportManager`.
4. **Firmware restore**: the sequence moves to `FirmwareRestore`.
5. Reduce `RobotController` to the facade and delete what became dead code.

Estimate: several sessions in total.

## Tests

In `src/core/tests/`, linking only `Zowi::core` (core must stay Qt-free):

- A **fake `BluetoothApi`** that records what is sent and lets the test inject
  incoming data and connection changes (including frames split in two chunks, as
  seen on real Bluetooth links: `"&"` then `"&F%%"`).
- A **virtual clock**.
- Cases that today can only be checked with a robot: reconnect after a lost link,
  connect timeout falling back to Demo, USB identity mismatch, transport lost,
  switching and reverting transport, low battery warning once per drop, restore with
  low battery (confirm / cancel), restore failure paths.

## Risks and how to cover them

- **Regression in the connection logic.** It is the most delicate part of the app.
  Cover it with the tests above, keep each step small and compare behaviour before
  and after.
- **No hardware in automated tests.** Finish with a manual pass using a real robot,
  over Bluetooth and over USB: first-time wizard, rename, reconnect, forget/unregister,
  transport lost, low battery, and firmware restore.
- **Stale QML caches while testing the GUI.** Delete `~/.cache/ZowiDesktop` after a
  rebuild when the version number does not change.
- **A big uncommitted working tree makes a refactor hard to review.** Start from a
  clean commit.

## Out of scope

- The online ranking network client (the host owns the transport by design).
- Changing any behaviour, protocol or UI.
- Splitting the QML-facing `Robot` object into several context objects.
