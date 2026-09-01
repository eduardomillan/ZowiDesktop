# Zowi Desktop

**Zowi Desktop** is a cross-platform application to control and program your Zowi robot from a computer.

Zowi is an open-source quadruped robot designed for education. It walks, dances, reacts to its environment, and helps children and beginners learn the basics of robotics and programming in a fun and hands-on way.

This desktop app lets you connect to Zowi via Bluetooth or USB, control its movements, drive it with a game-pad style control, flash new firmware, and rename it — all without needing an Android device or a mobile phone.

Whether you already own a Zowi or you are just curious about robotics, Zowi Desktop is your companion to explore, play, and learn.

![Splash screen](https://eduardomillan.github.io/ZowiDesktop/screenshots/splash_screen.png)

![Welcome screen](https://eduardomillan.github.io/ZowiDesktop/screenshots/welcome_screen.png)

---

## Features

- **Bluetooth & USB connection** — pair and connect over Bluetooth (BlueZ SPP) or plug in over USB serial, without needing root.
- **Wizard onboarding** — scan for nearby Zowis, pick one, and rename it from the first-run wizard.
- **Movement control** — a full movement pad to walk, dance and turn; battery and firmware status shown live.
- **Firmware flashing** — restore the factory firmware or install alternative firmwares (e.g. Alarm, Adivinawi) over Bluetooth or USB.
- **Internationalized** — available in Spanish (`es_ES`), Catalan (`ca_ES`), English (`en_US`), French (`fr_FR`) and Bulgarian (`bg_BG`).
- **Command-line companion** — the included `zowi_cli` tool exposes the same core over a scriptable interface for automation and debugging.

![Home screen](https://eduardomillan.github.io/ZowiDesktop/screenshots/home_screen.png)

![Scan screen](https://eduardomillan.github.io/ZowiDesktop/screenshots/scan_screen.png)

![Wizard](https://eduardomillan.github.io/ZowiDesktop/screenshots/wizard_screen.png)

---

## Installation

See [INSTALL.md](INSTALL.md) for all supported installation options (AppImage, Debian packages, apt repository, and Windows installer/portable).

## Building from source

Cross-platform build with Qt (C++ core + QML GUI):

- **Linux / macOS**: `./build.sh` (GUI + CLI), or `./build.sh --cli` for the CLI only.
- **Windows**: open an **x64 Native Tools Command Prompt for VS 2022** and run `build.bat`.

Full build details live in [docs/project/BUILD.md](docs/project/BUILD.md).

## Releases

Releases are manual. See [docs/project/RELEASE.md](docs/project/RELEASE.md) for the end-to-end guide on building the Linux (AppImage + Debian) and Windows (portable + installer) artifacts, publishing the GitHub Release, and updating the signed apt repository.

## Testing

```bash
./run-tests.sh            # build + white-box + black-box test suites
./run-tests.sh --verbose  # also print the black-box CLI detail
```

Hardware-dependent tests (real robot over Bluetooth/USB) are opt-in; see [src/cli/tests/README.md](src/cli/tests/README.md).

## Command-line tool

`zowi_cli` mirrors the GUI's core over the terminal:

```bash
zowi_cli scan                 # look for nearby Zowis
zowi_cli connect B4:9D:0B:32:41:0E   # connect and read name/appId/battery
zowi_cli session list         # inspect saved session data
zowi_cli translate -l es_ES -s "Hola mundo"
```

See [docs/project/ZOWI_CLI_HOWTO.md](docs/project/ZOWI_CLI_HOWTO.md) for the full reference.

---

Built with Qt and QML (C++ core, Qt-free business logic).

Open source — contributions welcome.
