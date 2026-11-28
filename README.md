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
- **Timeline editor** — build multi-step routines ("1, 2, 3 ¡Acción!") by chaining movements, gestures and mouth expressions with configurable repetitions, speed, and direction. Play back your sequences on the robot with live visual feedback (animated highlight, auto-scroll).
- **Mouth & gesture picker** — open dedicated screens from the gamepad to pick any LED-matrix mouth expression or body gesture (responsive grid, sends `L`/`H` commands) with just a tap.
- **Servo calibration** — fine-tune the servo trims from the GUI (interactive 4-step wizard) or the CLI (wizard or direct mode) to keep Zowi standing straight.
- **Firmware flashing** — restore the factory firmware or install alternative firmwares (e.g. Alarm, Adivinawi) over Bluetooth or USB.
- **Games and ranking** — play Memory (Zowi says), Draw the mouths and Timeline; each adds your best score to a single local ranking of `Player-NNN` players, reachable from the home screen. An optional, opt-in world ranking is available.
- **Discover projects** — guided lessons about the robot (gravity, eyes, feet, mouth, choreography…) with a quiz, and projects that reprogram the robot.
- **Achievements** — 17 collectible achievements (movements, animations, a game) earned by playing, passing project quizzes and using the app; some unlock gamepad movements and the Mouths Editor. An administrator can switch the layer off or stop it from locking content.
- **Low battery warnings** — the status bar flags a low battery and the app asks before installing firmware when the charge is low (threshold configurable).
- **Internationalized** — available in Spanish (`es_ES`), Catalan (`ca_ES`), English (`en_US`), French (`fr_FR`) and Bulgarian (`bg_BG`); change the language any time from Settings.
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

Built with Qt and QML (C++ core, Qt-free business logic). The interface uses the
[Noto Sans](https://fonts.google.com/noto) font (SIL Open Font License 1.1, see
[fonts/OFL.txt](fonts/OFL.txt)).

Open source — contributions welcome.

## Privacy

Zowi Desktop works locally: no accounts, advertising, analytics or profiling. It never
asks for personal data. It only stores your robot's pairing details, settings, your
Timeline sequence and the local ranking (random `Player-NNN` numbers) in files in your
user folder, plus technical logs. The local ranking cannot be deleted from inside the
app: only an administrator can wipe it (`zowi_cli ranking clear`).

The only thing that uses the Internet is the **optional world ranking**. Opening its
tab downloads the public top 100. Sharing your score is **off by default**: if you tick
the box, only your `Player-NNN` and your points are sent, you can delete your entry at
any time from the same window, and it expires after 30 days without improvement. An
administrator can switch the ranking, or just its online part, off for a whole
installation (`ranking_enabled`, `ranking_online_allowed`; see
[docs/project/CONFIG_HOWTO.md](docs/project/CONFIG_HOWTO.md)).

The full policy (in Spanish and English, **draft pending review**) is in
[docs/project/PRIVACY.md](docs/project/PRIVACY.md).

## Project documentation

- [Project index](docs/project/INDEX.md)
- [Screens index](docs/project/screens/INDEX.md)
