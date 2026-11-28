# Web Version — Design Document

> Frontier design doc: nothing is implemented yet. This document collects the
> technical feasibility analysis, the recommended strategy and a phased roadmap
> for shipping a web frontend for Zowi Desktop. It is a living document — update
> it as decisions are made and work progresses.

## Table of contents

- [Purpose & status](#purpose--status)
- [Technical feasibility](#technical-feasibility)
  - [What is already web-ready](#what-is-already-web-ready)
  - [Hard constraint: Bluetooth Classic](#hard-constraint-bluetooth-classic)
  - [What the browser can reach](#what-the-browser-can-reach)
- [Strategy](#strategy)
- [Frontend options: Qt for WebAssembly vs Vue/React](#frontend-options-qt-for-webassembly-vs-vuereact)
- [What the web reuses and what it must rewrite](#what-the-web-reuses-and-what-it-must-rewrite)
- [Roadmap](#roadmap)
  - [Phase 0 — Move the controller logic into core](#phase-0--move-the-controller-logic-into-core)
  - [Phase 1 — Core to WASM](#phase-1--core-to-wasm)
  - [Phase 2 — Hardware-free web demo](#phase-2--hardware-free-web-demo)
  - [Phase 3 — Real hardware from the browser](#phase-3--real-hardware-from-the-browser)
- [Effort and what is needed from the maintainer](#effort-and-what-is-needed-from-the-maintainer)
- [Repository & pipelines](#repository--pipelines)
- [Relationship with the repository strategy](#relationship-with-the-repository-strategy)
- [Deferred decisions](#deferred-decisions)

## Purpose & status

Goal: offer Zowi Desktop **as a web frontend**, following the same
**API + CLI + Frontend** architecture already in place — the web becomes a
third consumer of `zowi_core`, compiled to WebAssembly.

**Status: not started.** Everything in this document is analysis and proposal.
No implementation exists in the repository (there is no web target, no
Emscripten build, no CI job, no frontend scaffolding).

## Technical feasibility

### What is already web-ready

`zowi::core` (`src/core/`) is **WASM-ready**, a direct consequence of the
Qt-free refactor:

- It has **no Qt dependency**; the only third-party dependency is
  `nlohmann/json` (header-only).
- It uses only C++ standard library for I/O (`std::fstream` for `SessionStore`,
  the default translation loader and `ConfigStore`); no POSIX/Win32/Boost
  platform API.
- The injectable seams created by the refactor map 1:1 to browser equivalents:

  | Core seam (injectable) | Browser adapter |
  |---|---|
  | `TranslationEngine::setTranslationLoader(...)` | `fetch("i18n/zowi_<locale>.json")` |
  | `SessionStore(configDir)` | `localStorage` / IndexedDB adapter |
  | `TranslationEngine::setLogCallback(...)` | `console.log` / `console.warn` |

  The translation files are plain `.json` under `i18n/`, already served verbatim
  by any static host — no build step needed on the server side.

So the business logic — protocol, commands, movements, calibration, i18n,
session — is fully portable to WebAssembly with **no core changes**.

### Hard constraint: Bluetooth Classic

The Zowi robot connects over **Bluetooth Classic SPP** (HC-05/HC-06 module).

The **Web Bluetooth API supports BLE only**, not Classic SPP. Therefore a
browser **cannot talk to a real Zowi over Bluetooth**, period. No library,
framework or WASM port changes this — it is a platform limitation.

Implication: Bluetooth control from a web frontend requires a **local bridge**
(a small daemon owning the BT connection and exposing it to the browser), never
a direct in-browser connection.

### What the browser can reach

| Transport | Browser capability | Feasible |
|---|---|---|
| Bluetooth Classic SPP | Web Bluetooth API = BLE only | ❌ direct; ✅ via local bridge (Phase 3) |
| USB / serial | **Web Serial API** (Chromium-based browsers) | ✅ control + STK500 flashing (Phase 3) |
| No hardware (simulator) | `VirtualZowiBackend` (roadmap item) | ✅ fully in-browser (Phase 2) |

## Strategy

The web is a **third consumer of `zowi_core` compiled to WASM**, mirroring how
`zowi_cli` and the Qt GUI consume it today. Core stays the single source of
truth; the frontend is a thin adapter layer, exactly like the GUI adapters in
`src/gui/controllers/`.

```
┌────────────────────────────────────────────────────────────┐
│                        Consumers                            │
│                                                             │
│  ┌──────────────┐ ┌──────────────┐ ┌────────────────────┐  │
│  │  zowi_cli    │ │ ZowiDesktop  │ │  Web frontend      │  │
│  │  (CLI)       │ │ (Qt 6 / QML) │ │ (React or Vue +    │  │
│  └──────┬───────┘ └──────┬───────┘ │  TS, WASM)         │  │
│         │                │          └─────────┬──────────┘  │
│         │                │                    │             │
│         │         ┌──────┴────────────────────▼───┐         │
│         └────────►│          zowi_core (WASM)      │◄────────┘
│                   └──────┬────────────────┬───────┘         │
│                          │                │                 │
│              ┌───────────▼───┐  ┌─────────▼────────┐        │
│              │ backends (host│  │ browser adapters │        │
│              │ Qt/Win/POSIX) │  │ fetch/localStorage│        │
│              └───────────────┘  └──────────────────┘        │
└────────────────────────────────────────────────────────────┘
```

Key rules:

- **Core is untouched** for the web; all browser-specific work lives in a
  dedicated web layer (adapters + frontend), the same way the GUI is an adapter
  layer, not a second business-logic layer.
- **Frontend technology is deferred** (TBD): Qt for WebAssembly (reusing the QML
  app), or React + TypeScript + Vite, or Vue 3 + TypeScript + Vite, all viable — see
  [Deferred decisions](#deferred-decisions).
- **Screens are already specified**: the QML screen docs under
  [docs/project/screens/](screens/INDEX.md) (`SCREEN_*.md`) document every
  screen's behaviour, navigation and context objects; they are the spec for the
  web frontend regardless of framework.

## Frontend options: Qt for WebAssembly vs Vue/React

There are two ways to build the Phase 2 frontend. Both sit on top of the same
WASM core ideas and both need a new robot backend for the browser (see
[Phase 3](#phase-3--real-hardware-from-the-browser)). This section is analysis
only; **neither has been tried in this repository**.

### What Qt for WebAssembly is

One of Qt's official platforms: it compiles a Qt application (here the existing
QML app) to WebAssembly with Emscripten. The result is a `.wasm`, a bit of
JavaScript glue and an HTML page that loads it; the app runs in the tab like a
native program, with no rewrite. Traits that drive every trade-off:

- It **draws on a `<canvas>`** (WebGL), not in the DOM: Qt paints its own pixels
  instead of generating HTML.
- It is **statically linked**: all of Qt ships inside the package, and each Qt
  release pins one specific Emscripten version.
- It is a browser without native hardware: **no Bluetooth, no `QtSerialPort`**,
  and only a virtual in-memory file system.

### Qt for WebAssembly vs Vue / React

| | Qt for WebAssembly | Vue / React (+ TS, Vite) |
|---|---|---|
| Reuse of the current app | The ~20 QML screens, the components (status bar, Splash, flags), the C++ controllers and the translations, almost untouched | Everything rewritten in TypeScript, using `SCREEN_*.md` as the spec |
| Maintenance | One codebase for desktop and web: a fix or a redesign serves both | Two UIs to keep in sync (and two sets of bugs) |
| Look | Identical to the desktop; the same preview/capture checks apply | Its own implementation, to be matched by hand |
| Size and startup | Several MB (probably tens before compression), slow first load | Very light |
| Accessibility, text selection, page translation | Limited (it is a canvas) | Native browser behaviour |
| Mobile and virtual keyboard | More problematic | Normal |
| Web APIs (Web Serial, WebSocket) | Needs a JS<->C++ bridge | Direct |
| Hosting | Static, but multithreaded builds need special headers (COOP/COEP) | Any static host |
| Iteration and debugging | Slower | Very fast |

Two further points for Qt: Qt's licence (LGPL) combined with static linking must
be checked against the project's licence (normally fine for a public repository,
but to be confirmed), and the desktop Bluetooth/USB backends do not apply: a new
backend that talks to the browser (Web Serial over USB, or a WebSocket to the
local bridge) has to be plugged into the same `RobotController`. The same
backend work is needed with Vue/React; the difference is that with Qt the bridge
crosses C++ and JavaScript.

### When to choose each

- **Qt for WebAssembly** — a quick demo that is exactly the desktop app, accepting
  a heavy package. Cheapest way to get something playable.
- **Vue / React** — the web as a product of its own: fast to load, usable on
  phones and accessible, accepting the cost of rebuilding the interface.

If undecided, a short spike with Qt for WebAssembly (one screen, for instance the
Splash) would show its weight and load time quickly, before committing to either.

## What the web reuses and what it must rewrite

Assuming a Vue/React frontend (with Qt for WebAssembly most of the GUI column
below is reused instead):

| Layer | Web | Notes |
|---|---|---|
| `src/core` | **Reused, untouched** | Compiled to WebAssembly and exposed to the browser through a thin wrapper (embind or a C API). Same protocol, commands, calibration, translations, games and achievements as the desktop and the CLI. |
| `src/firmware` (STK500v1) | **Reusable** | Standard C++ with an injected transport: it could be compiled to WebAssembly and fed by Web Serial to flash over USB from the browser. |
| `src/gui`, `src/views` | **Rewritten** | The QML screens become Vue components, with `SCREEN_*.md` as the spec. |
| `src/cli` | **Not used in the browser** | A native program (Qt, system backends). It stays as it is. Indirectly it is the base of the local Bluetooth bridge (see Phase 3): the shell's command execution is extracted and exposed over WebSocket. |

The delicate part is `src/gui/controllers`. They are Qt/C++ and **not in core**, but
`RobotController` (about 1,400 lines) holds logic the web needs just as much: the
situation state machine (Demo / Unregistered / Connecting / Connected /
Disconnected / TransportLost), the identity polling, the connect timeout and the
USB identity check, the low-battery rule, the transport switching and the firmware
restore sequence. Most of the other controllers are thin adapters over core
modules. Two ways to get that logic into the web:

- **Rewrite it in TypeScript:** faster to start, but the logic ends up duplicated
  and has to be kept in sync.
- **Move it into core first (recommended):** more up-front work, but desktop,
  CLI and web then share one copy. It matches the architecture rule that the GUI is
  an adapter layer and not a second business-logic layer. See
  [Phase 0](#phase-0--move-the-controller-logic-into-core).

The same applies to flows that live in the CLI today, such as the readiness check
before sending an order to the robot (`waitForRobotReady`).

## Roadmap

### Phase 0 — Move the controller logic into core

Prerequisite for any web frontend (and a benefit on its own: more of the logic
becomes testable without Qt or hardware). The goal is a behaviour-preserving
refactor: `RobotController` becomes a thin Qt adapter (properties, signals, timers)
over a Qt-free core class.

- A core class (working name `RobotSession`) built on `BluetoothApi` that owns:
  the situation state machine (`computeSituation` becomes a pure function of its
  inputs), the low-battery rule, the connect timeout and the USB identity check, the
  identity polling cadence, transport selection and the firmware restore sequence
  (including the battery check).
- Timers and the clock are **injected** (a small interface), so that the core can
  run on Qt timers, in a test with a virtual clock, or in the browser.
- Tests in `src/core/tests/` with a fake `BluetoothApi` and a virtual clock cover the
  transitions that today can only be checked with a robot (reconnect, timeout,
  identity mismatch, transport lost, restore).
- Do it in small steps, each one building and passing the tests: first the pure
  pieces (situation, low battery, connect timeout), then the transport/USB probe
  logic, finally the restore orchestration.
- Out of scope by design: the online ranking network client (the host owns the
  transport, see `ranking_online`) and anything that talks to Qt or the operating
  system directly (serial port enumeration, temporary files).
- Validation gate: besides the automated tests, a manual pass with a real robot
  over Bluetooth and USB (wizard, reconnect, unregister, restore).

### Phase 1 — Core to WASM

Foundations: make `zowi_core` buildable and testable in the browser.

- Emscripten build target for `src/core/` (new `src/web/` layer or a CMake
  toolchain file + `wasm` preset) producing a static `.wasm` + JS glue.
- A thin **embind** wrapper (or C API + `.d.ts`) exposing the core classes:
  `SessionStore`, `TranslationEngine`, `ConfigStore`, `RobotCommands`,
  `RobotState`, `MessageParser`, `MovementSequencer`, `CalibrationSession`,
  `protocol`.
- Browser adapters: translation loader via `fetch`, session persistence on
  `localStorage`/IndexedDB, log callback to `console`.
- **Validation gate**: the existing 8 core tests (`src/core/tests/`) compiled to
  WASM and run under Node.js or a headless browser; a CI job (`web-wasm`) keeps
  core web-green on every push.
- Deliverable: `zowi-core-wasm` artifact + documented integration API.

### Phase 2 — Hardware-free web demo

Highest value / lowest effort: a fully in-browser experience **without any
robot**, powered by the `VirtualZowiBackend` roadmap item
([docs/project/ZOWILIBS.md](ZOWILIBS.md) — "Virtual Zowi (simulator)"),
currently proposed at ~150–250 lines of core code + a robot-side command parser.

- Implement `VirtualZowiBackend : BluetoothApi` in core (also benefits the
  desktop: closed round-trip integration tests, GUI demo mode).
- Web frontend (Qt for WebAssembly, Vue or React: see
  [Frontend options](#frontend-options-qt-for-webassembly-vs-vuereact)) implementing
  the screens already specified in
  `SCREEN_*.md`, starting with the hardware-independent ones:
  - Pad (gamepad), Mouth picker, Gesture picker — via `RobotCommands`.
  - Mouth editor (pintabocas).
  - Calibration screen (servo trims).
  - The games: Adivinawi, Memory (Timeline), Draw-the-mouth.
  - Locale switcher over the 5 existing languages (`es_ES`, `ca_ES`, `en_US`,
    `fr_FR`, `bg_BG`).
- Deploy as a static site (decision pending — see
  [Deferred decisions](#deferred-decisions)); a `gh-pages/docs/` sub-path is
  safe as long as it never touches the apt repo (`docs/dists`, `docs/pool`,
  `docs/keyring.gpg`) or the website files.
- Deliverable: playable, shareable Zowi-in-the-browser demo.

### Phase 3 — Real hardware from the browser

- **USB (Web Serial API, Chromium)**: adapt `SerialBluetoothBackend`
  (`src/backends/bt_serial/`) to browser serial; enables live control and
  STK500v1 firmware flashing from the browser. Most complex phase; do after
  Phase 2 matures.
- **Bluetooth (local bridge)**: inherit the deferred *daemon mode* already
  specced in [docs/project/ZOWI_CLI_SHELL.md](ZOWI_CLI_SHELL.md)
  ("Future work: daemon mode"): factor shell command execution into
  `executeShellCommand(BluetoothApi &, line)`, add a daemon exposing
  `WebSocket`, and let the frontend drive a real BT Zowi over `localhost`.
  This keeps all host-side Bluetooth logic in the existing stack.

Deliverable: control a physical Zowi (USB and/or Bluetooth via bridge) from the
web frontend.

## Effort and what is needed from the maintainer

Rough estimates, expressed in working sessions (not dates: they depend on how many
review rounds each screen needs, as in the status bar and Splash redesigns):

| Piece | Estimate |
|---|---|
| Phase 0 — controller logic into core | Several sessions, in small verified steps (the first one, the situation machine / low battery / connect timeout, is about one session); needs a final test with a real robot |
| Phase 1 — core to WASM | One or a few sessions |
| Phase 2 — web demo with Vue/React | The big one (~20 screens): from several long sessions to many more, depending on visual polish |
| Phase 2 — web demo with Qt for WebAssembly | Considerably less (the UI is reused), plus the browser backend and the packaging |
| Phase 3 — real hardware | A separate, delicate part, especially flashing over USB |

What the maintainer has to provide:

- **Design decisions:** framework (Qt wasm, Vue or React), where to publish it
  (`gh-pages/docs/` without touching the apt repo, or another domain), the scope of
  each phase, and whether to start with the hardware-free demo only.
- **Environment:** install Emscripten and Node (and, for Qt, the Qt for WebAssembly
  kit); authorise anything that asks for permissions.
- **Real-hardware testing:** the UI can be checked with screenshots, but a real
  robot, Web Serial and the Bluetooth bridge can only be tested by the maintainer,
  following instructions.
- **Visual and usability validation:** as with the proofs of concept, the look is
  approved before going on.
- **Publishing and versioning:** commits and deployments are made by the maintainer
  (or explicitly requested), as `AGENTS.md` requires.

## Repository & pipelines

Proposed layout (final placement to be confirmed with the maintainer):

```
src/web/               # Web layer: WASM build, adapters, frontend app
  ├── wasm/            # Emscripten target for zowi_core + embind wrapper
  ├── adapters/        # browser loaders (fetch, localStorage, console)
  └── frontend/        # React or Vue + TS app (framework TBD); not needed for Qt wasm
```

CI considerations:

- New job `web-wasm`: build `zowi_core` for WASM and run the core tests under
  Node/headless browser; fail the pipeline on regression.
- Static deploy job (Phase 2+) publishing the frontend artifact.
- The web layer is additive: it must never affect existing Linux/Windows builds
  or the manual release flow (`packaging/create-gh-release.sh`,
  `publish-apt-repo.sh`).

## Relationship with the repository strategy

Consistent with [docs/project/PLANNING.md](PLANNING.md) ("Repository strategy"):

- **Keep the monorepo for now.** The web work starts in this repository
  (`src/web/`), gated by its own CMake option and CI jobs.
- The web (and a future Android app) are exactly the real consumers that would
  justify extracting `zowi-core` into its own repository **later**, when the
  web phase 1 exists and multi-consumer release coordination becomes real. Do
  not split preemptively.

## Deferred decisions

| Decision | Options | Status |
|---|---|---|
| Frontend technology | **Qt for WebAssembly** — reuses the whole QML app; heavy package, canvas rendering (see [Frontend options](#frontend-options-qt-for-webassembly-vs-vuereact)) | TBD |
| | **React + TypeScript + Vite** — largest ecosystem, best TS/WASM tooling, Vite makes static multi-page + WASM trivial | TBD |
| | **Vue 3 + TypeScript + Vite** — lighter, simpler to learn, same Vite tooling, good WASM story | TBD |
| Deployment target | `gh-pages/docs/` sub-path (safe coexistence with apt repo + website) vs. dedicated site / repo (`zowi.app` idea) | TBD |
| Core exposure | embind wrapper vs. plain C API + `.d.ts` (affects ABI stability and how Android reuses the same lib) | TBD |
| Web layer placement | `src/web/` (in-repo) vs. tree `ports/emscripten` style | TBD |
| Scope of each phase | Phases 1–3 above; re-order or extend if a consumer asks for a specific slice first | TBD |