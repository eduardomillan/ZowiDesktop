# Analysis: Building a ZowiWebApp on top of bq/web2boardjs

## Summary of findings

- `avrgirl-arduino` (current v5.x) **still supports `bqZum`**, the ZUM board used by Zowi,
  along with `zumcore2` and `zumjunior`. Native firmware flashing therefore remains viable.
- The **live-control serial protocol for Zowi** lives in `bq/zowiLibs` in the
  `ZowiSerialCommand` library (serial commands sent to the Zowi firmware). It is NOT part of
  web2boardjs and must be reimplemented as a JS client.
- Zowi's firmware (servos/gaits) is in `bq/zowiLibs` (`Zowi.cpp`, `Oscillator`, `US`).
- `web2boardjs` is an Electron desktop app that exposes a `socket.io` server on port **9876**
  and acts as the native bridge between a web app (originally Bitbloq) and Arduino/Zowi
  hardware. It already handles `compile`, `upload`, serial port open/send/receive, `version`,
  etc. Zowi is mapped via `board.mcu = 'bt328'` -> `'bqZum'` in `uploader.js`.

## Decision: native bridge (fork web2boardjs), not pure Web Serial

Flashing Zowi's bootloader requires native `avrdude`/`avrgirl` tooling that the browser
cannot perform reliably. Web Serial could only serve as a fallback for the *live-control*
path, not for flashing. Therefore the recommended approach is to **fork and modernize
web2boardjs** as the installed native bridge, and build the `ZowiWebApp` as the web front-end
that talks to it over `socket.io`.

## Architecture

```
ZowiWebApp (browser)  --socket.io (ws://localhost:9876)-->  web2boardjs (Electron, installed on PC)  --USB/Serial (CP210x)-->  Zowi
```

## Plan

### Phase 0 - Control MVP (fast, high value)
1. Fork `web2boardjs`. Update `package.json`: Electron v28+, `socket.io` (with explicit
   `io.origins([web-origin])`), `avrgirl-arduino` ^5, `serialport` ^11, `electron-log`.
2. Keep `main.js` mostly as-is (socket on `:9876`) but add explicit CORS for the web origin.
3. Minimal `ZowiWebApp` (HTML+JS) that connects via `socket.io` to `ws://localhost:9876`,
   opens the port (`openserialport`) and sends `ZowiSerialCommand` protocol commands to a
   Zowi **that already has firmware**.

### Phase 1 - Firmware flashing
4. Use `bq/zowiLibs` (firmware `.ino` + `ZowiSerialCommand`) as the source.
5. `compiler.js`: upgrade to modern `arduino-cli` / `arduino-builder` pointing at `zowiLibs`.
6. `uploader.js`: already maps `bt328` -> `bqZum`; confirm reset via DTR/RTS with `avrgirl` 5.
   `uploadchild.js` stays the same.
7. Add a "Compile + Upload" endpoint/UI in the web app.

### Phase 2 - Full ZowiWebApp
8. Front-end (proposed: React + Vite): code editor (Monaco/CodeMirror) or blocks (Blockly),
   Compile/Upload/Serial-monitor buttons, and a movement panel (gaits: walk, turn, swing, ...).
9. JS client for the `ZowiSerialCommand` protocol (command/response parser).
10. Bidirectional serial monitor (`serialportdata` emitted from `socket`).

### Phase 3 - Distribution
11. `electron-builder` (win/mac/linux) with Zowi drivers (SiLabs CP210x) bundled and the
    `web2board://` protocol to launch from the web.
12. Solve signing: macOS notarization and Windows SmartScreen (original drivers may cause
    issues on modern macOS).

## Risks / caveats
- **Firmware version**: the `ZowiSerialCommand` protocol varies between Zowi firmware
  versions; pin a specific `bq/zowiLibs` version.
- **Modern macOS** blocks unsigned drivers/kexts; USB CP210x control may require manual steps.
- Some Zowi models use **BLE**; this flow assumes **USB/serial** (via CP210x), which is what
  web2boardjs supports.

## Assumptions
- Connect over **USB/serial**, not BLE.
- Front-end in **JavaScript (React + Vite)** unless otherwise specified.
- The user installs the native bridge (Electron app) on their machine; the web app cannot
  flash firmware on its own.
