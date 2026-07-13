# Development Plan — Zowi Desktop

> This is a living document. Edit it to reflect the actual roadmap and priorities.

## Release Plan

| Version | Milestone | Description | Status |
|---------|-----------|-------------|--------|
| **0.1.0** | M1 | Project scaffold (screens, i18n, config, session) | ✅ |
| **0.1.1** | M1 | + First working Windows portable build | ✅ |
| **0.2.0** | M2 | Bluetooth discovery + device list | 🔧 |
| **0.2.1** | M2 | Pairing + SPP connection | |
| **0.2.2** | M2 | Connection status indicator | |
| **0.3.0** | M3 | Basic control pad (directions + actions) | |
| **0.3.1** | M3 | Speed control + face editor | |
| **0.4.0** | M4 | Demo / autonomous mode | |
| **0.4.1** | M4 | Guardian + dancing + sound modes | |
| **0.5.0** | M5 | Firmware flashing (STK500v1) | |
| **0.6.0** | M6 | Visual block editor | |
| **0.7.0** | M7 | Projects & tutorials section | |
| **0.8.0** | M8 | Full i18n + polish | |
| **1.0.0** | — | Final release | |

## Milestones

### M1 — Project scaffold
- [x] CMake + Qt6 project building and linking
- [x] QML resource system with images and translations
- [x] Custom `Translator` class reading `.ts` files (`es_ES`, `ca_ES`, `en_US`)
- [x] `SessionController` persisting wizard state and active Zowi address
- [x] Splash screen (`SplashScreen.qml`) with language flags and Continue/Quit
- [x] Start screen (`StartScreen.qml`) with Start button and know-more link
- [x] Welcome screen (`WelcomeScreen.qml`) with onboarding choices
- [x] Scan screen (`ScanScreen.qml`) with Bluetooth device discovery
- [x] Windows portable build (cross-compiled from Linux)
- [x] Linux AppImage packaging

### M2 — Bluetooth connection & device pairing
- [x] Device discovery list with name, address, signal strength
- [ ] Implement `QBluetoothSocket` (RFCOMM SPP, UUID `00001101-0000-1000-8000-00805F9B34FB`)
- [ ] Pairing flow and persistent storage of paired device
- [ ] Connection status indicator (connected / disconnected / low battery)

### M3 — Zowi control pad
- [ ] Directional movement pad (forward, backward, turn left/right)
- [ ] Action buttons (bend, crusaito, flapping, jitter, shake leg, swing, updown, tip-toe, moonwalker, etc.)
- [ ] Speed control (slow / medium / fast)
- [ ] Face/mouth editor
- [ ] Real-time command sending over Bluetooth

### M4 — Pre-programmed modes
- [ ] Demo / autonomous behaviour mode
- [ ] Guardian mode (obstacle detection)
- [ ] Dancing mode
- [ ] Sound-reactive mode (clap detection)

### M5 — Firmware flashing (STK500v1)
- [ ] Read `.hex` firmware file
- [ ] STK500v1 protocol over Bluetooth SPP
- [ ] Flash progress UI
- [ ] Factory reset / restore original firmware

### M6 — Visual programming (block editor)
- [ ] Block palette with Zowi-specific blocks (move, turn, dance, face, etc.)
- [ ] Drag-and-drop workspace
- [ ] Code generation → send to robot
- [ ] Save / load programs

### M7 — Projects & tutorial section
- [ ] Built-in project gallery (move, choreography, form, biology, gravity, paint, etc.)
- [ ] Step-by-step tutorial UI
- [ ] Achievements / badges

### M8 — Polish & i18n
- [ ] Full translation of all UI strings (es_ES, ca_ES, en_US)
- [ ] Window icon, app metadata
- [ ] Keyboard shortcuts
- [ ] Responsive layout (window resize)

## Technical notes

- Bluetooth Classic SPP only — BLE is not supported by Zowi's HC-06/HC-05 module.
- Firmware flashing uses the same STK500v1 protocol as the original Android app.
- Translations are loaded from `.ts` XML files at runtime (no `lrelease` needed).
- All robot images and UI assets come from the original Android project (`drawable-xxxhdpi`).
