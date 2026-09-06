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
- [Roadmap](#roadmap)
  - [Phase 1 — Core to WASM](#phase-1--core-to-wasm)
  - [Phase 2 — Hardware-free web demo](#phase-2--hardware-free-web-demo)
  - [Phase 3 — Real hardware from the browser](#phase-3--real-hardware-from-the-browser)
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
- **Frontend framework is deferred** (TBD): React + TypeScript + Vite or
  Vue 3 + TypeScript + Vite, both viable — see
  [Deferred decisions](#deferred-decisions).
- **Screens are already specified**: the QML screen docs under
  [docs/project/screens/](screens/INDEX.md) (`SCREEN_*.md`) document every
  screen's behaviour, navigation and context objects; they are the spec for the
  web frontend regardless of framework.

## Roadmap

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
- Web frontend (framework TBD) implementing the screens already specified in
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

## Repository & pipelines

Proposed layout (final placement to be confirmed with the maintainer):

```
src/web/               # Web layer: WASM build, adapters, frontend app
  ├── wasm/            # Emscripten target for zowi_core + embind wrapper
  ├── adapters/        # browser loaders (fetch, localStorage, console)
  └── frontend/        # React or Vue + TS app (framework TBD)
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
| Frontend framework | **React + TypeScript + Vite** — largest ecosystem, best TS/WASM tooling, Vite makes static multi-page + WASM trivial | TBD |
| | **Vue 3 + TypeScript + Vite** — lighter, simpler to learn, same Vite tooling, good WASM story | TBD |
| Deployment target | `gh-pages/docs/` sub-path (safe coexistence with apt repo + website) vs. dedicated site / repo (`zowi.app` idea) | TBD |
| Core exposure | embind wrapper vs. plain C API + `.d.ts` (affects ABI stability and how Android reuses the same lib) | TBD |
| Web layer placement | `src/web/` (in-repo) vs. tree `ports/emscripten` style | TBD |
| Scope of each phase | Phases 1–3 above; re-order or extend if a consumer asks for a specific slice first | TBD |