# ZOWI_DESKTOP_FW: the project's own firmware

> **Status: PLANNED — not implemented yet.** This document is the design and
> implementation plan for a ZowiDesktop-owned firmware, `ZOWI_DESKTOP_FW`,
> derived from BQ's `ZOWI_BASE_v2` (sibling repo
> [zowiLibs](https://github.com/eduardomillan/zowiLibs)). It is the canonical
> home of the mouth read-back design originally sketched in the former
> "Pending Firmware Extension" section of
> [FIRMWARE_HOWTO.md](../project/FIRMWARE_HOWTO.md); that section is now a
> summary pointing here.
>
> Related docs: [PROTOCOL.md](PROTOCOL.md) (wire protocol),
> [ZOWILIBS.md](../project/ZOWILIBS.md) (relationship with the sibling repo),
> [FIRMWARE_HOWTO.md](../project/FIRMWARE_HOWTO.md) (flashing over BT/USB).

## Table of contents

- [Motivation](#motivation)
- [Adopted decisions](#adopted-decisions)
- [Protocol extensions: `W` and `V`](#protocol-extensions-w-and-v)
- [Phase 1 — zowiLibs patch](#phase-1--zowilibs-patch)
- [Phase 2 — Firmware sources and build](#phase-2--firmware-sources-and-build)
- [Phase 3 — Core (Qt-free)](#phase-3--core-qt-free)
- [Phase 4 — GUI](#phase-4--gui)
- [Phase 5 — CLI](#phase-5--cli)
- [Phase 6 — Docs and verification scripts](#phase-6--docs-and-verification-scripts)
- [File summary](#file-summary)
- [Verification plan](#verification-plan)
- [Open issues](#open-issues)
- [Licensing](#licensing)

## Motivation

The stock firmware (`ZOWI_BASE_v2`) exposes the LED matrix as **write-only**:
`L <bits>` sets the mouth, and the query commands (`E`/`D`/`N`/`B`/`I`) do not
include the matrix. The value *is* stored on the robot — `LedMatrix` keeps the
current pattern in `LedMatrix::memory` and exposes `readFull()` — but nothing
sends it over serial, so the desktop app cannot ask "what mouth are you
showing?".

The original use case is the **pintabocas** (`MouthEditorScreen.qml`): open the
editor and preload the grid with the mouth the robot is showing at that moment,
instead of always starting from an empty grid. Client-side tracking (last `L`
sent by the app) was considered and rejected as the primary mechanism: it is
wrong whenever the mouth changes from a gesture (`H`), a game firmware, or a
power cycle.

Having our own firmware also gives the project:

- **Protocol ownership** — future extensions (sensor reads, richer queries)
  without waiting on zowiLibs.
- **Capability detection** — the app already reads the program id via
  `I` → `&&I <appId>%%`; a robot reporting `ZOWI_DESKTOP_FW` is known to
  support the extensions, and UI features can be gated on it.
- **A reproducible firmware build** in this repo (`arduino-cli`), with the
  compiled `.hex` committed like the other three firmware images.

## Adopted decisions

| Decision | Choice | Notes |
|---|---|---|
| Firmware name | **`ZOWI_DESKTOP_FW`** | `programID`, sketch folder, `.ino`, `.hex`, appId — all use this exact string. |
| Arduino libraries | **Not copied.** Imported at build time via `arduino-cli compile --libraries "$ZOWILIBS_PATH/arduinolibs"` | Same path convention as `scripts/sync_firmware_from_zowiLibs.sh` and `scripts/verify_arduino_mirrors.sh` (`$ZOWILIBS_PATH`, default `~/zowiLibs`). The `.hex` **is** committed, so building the desktop app never needs zowiLibs — only regenerating the firmware does. |
| zowiLibs changes | **Patch file committed in this repo** (`src/firmware/patches/zowiLibs_read_mouth.patch`); the maintainer applies it in zowiLibs and pushes there | The library changes are unavoidable (see Phase 1): the command table is full and `Zowi::ledmatrix` is private. |
| Mouth read command | **`W`** — the letter agreed in the original design note (formerly the "Pending Firmware Extension" section of FIRMWARE_HOWTO.md, now folded into this document) | Letter free; response `&&W <30 bits>%%`, symmetric with `L` for exact round-trips. |
| Single-LED read command | **`V <row> <col>`** | Letter free; response `&&V <0\|1>%%`. 1-based coordinates like `LedMatrix::setLed`. |
| Pintabocas behaviour | **Auto-preload on open + manual "Load current mouth" button** | Only when connected and `Robot.appId === "ZOWI_DESKTOP_FW"`; otherwise the grid starts empty as today. |
| Toolchain | **arduino-cli**, FQBN `arduino:avr:nano` | The firmware uses `A6` (Nano-only pin). `cpu=atmega328old` must be confirmed against the robot's actual bootloader — see [Open issues](#open-issues). |

## Protocol extensions: `W` and `V`

Both are **additive**: no existing command or response frame changes, so a
`ZOWI_DESKTOP_FW` robot remains fully compatible with everything the app
already does (and with the original BQ app).

### `W` — read full mouth (LED matrix)

```text
→ W\r
← &&W <30 binary digits>%%      # MSB first, zero-padded, e.g. smile:
                                # &&W 000000100001010010001100000000%%
```

- 30 bits (5 rows × 6 columns), bit 29 = row 1 / column 1 … bit 0 = row 5 /
  column 6 (the layout documented in ZOWILIBS.md). Bits 30–31 of
  `LedMatrix::memory` are unused.
- **Symmetric with `L`**: the value returned by `W` can be fed straight back
  to `L` for an exact round-trip.
- **No side effects**: the handler must *not* call `zowi.home()` (unlike
  `requestDistance`/`requestBattery`); reading the mouth must not stop the
  robot. No `&&A%%`/`&&F%%` acks — same style as `E`/`I`/`B`/`D`/`N`.

### `V` — read single LED

```text
→ V <row> <col>\r               # row 1..5, col 1..6 (1-based)
← &&V <0|1>%%
```

- Mirrors `LedMatrix::readLed(row, column)` / `setLed` coordinate semantics:
  bit `MATRIX_LENGTH - (row-1)*COLUMNS - col` of `memory`.
- Out-of-range arguments → `&&V 0%%` (no error frame exists in the protocol).
- **No side effects**, same as `W`.

### Firmware handlers (for the `.ino`)

`requestMouth()` is inherited from the original design note (formerly in
FIRMWARE_HOWTO.md); `requestLed()` follows the same pattern:

```cpp
//-- in setup(), next to the other request commands:
SCmd.addCommand("W", requestMouth);
SCmd.addCommand("V", requestLed);

void requestMouth(){
    unsigned long mouth = zowi.getMouth();
    Serial.print(F("&&"));
    Serial.print(F("W "));
    for(int i=29; i>=0; i--){        // 5x6 = 30 bits, MSB first
        Serial.print((mouth >> i) & 1UL);
    }
    Serial.println(F("%%"));
    Serial.flush();
}

void requestLed(){
    char *argRow = SCmd.next();
    char *argCol = SCmd.next();
    int row = (argRow != NULL) ? atoi(argRow) : 0;
    int col = (argCol != NULL) ? atoi(argCol) : 0;
    bool on = false;
    if (row >= 1 && row <= 5 && col >= 1 && col <= 6)
        on = zowi.readMouthLed((char)row, (char)col);
    Serial.print(F("&&"));
    Serial.print(F("V "));
    Serial.print(on ? '1' : '0');
    Serial.println(F("%%"));
    Serial.flush();
}
```

## Phase 1 — zowiLibs patch

New file: **`src/firmware/patches/zowiLibs_read_mouth.patch`** (git-diff
format, applied inside a zowiLibs checkout with
`git apply /path/to/zowiLibs_read_mouth.patch`). Application instructions go
in the firmware README (Phase 2). Contents — three additive changes:

1. **`arduinolibs/ZowiSerialCommand/ZowiSerialCommand.h`** — raise
   `MAXSERIALCOMMANDS` from `14` to `20`.
   *Why:* `ZOWI_BASE_v2.ino` already registers exactly 14 commands and
   `addCommand()` silently drops anything past the limit
   (`ZowiSerialCommand.cpp:106`), so `W`/`V` would never be registered. The
   `#define` has no `#ifndef` guard, so it cannot be overridden from the
   sketch.
2. **`arduinolibs/LedMatrix/LedMatrix.cpp`** — implement `readLed(row, col)`.
   *Why:* it is **declared** in `LedMatrix.h` (line 60) but **never defined** —
   using it today would fail at link time. Implementation mirrors `setLed`'s
   bit math:

   ```cpp
   bool LedMatrix::readLed(char row, char column) {
       if(row >= 1 && row <= ROWS && column >= 1 && column <= COLUMNS) {
           return (memory >> (MATRIX_LENGTH - (row-1)*COLUMNS - column)) & 1L;
       }
       return false;
   }
   ```
3. **`arduinolibs/Zowi/Zowi.h` + `Zowi.cpp`** — public accessors:

   ```cpp
   //-- Zowi.h (public section, next to putMouth/clearMouth):
   unsigned long getMouth();
   bool readMouthLed(char row, char column);

   //-- Zowi.cpp:
   unsigned long Zowi::getMouth(){ return ledmatrix.readFull(); }
   bool Zowi::readMouthLed(char row, char column){ return ledmatrix.readLed(row, column); }
   ```
   *Why:* `ledmatrix` is a **private** member (`Zowi.h:98`) and the class has
   no getter (`getMouthShape(int)` returns a preset by id, not the live
   memory). Gestures (`H`) change the mouth inside the library, so only the
   library can report the authoritative value.

Note: these changes also benefit the stock `ZOWI_BASE_v2`; whether the base
sketch itself registers `W`/`V` is a separate zowiLibs-side decision. This
project does not depend on it.

## Phase 2 — Firmware sources and build

New files:

```text
src/firmware/ZOWI_DESKTOP_FW/ZOWI_DESKTOP_FW.ino    # sketch (folder name must match, arduino-cli rule)
src/firmware/ZOWI_DESKTOP_FW/README.md              # build/flash instructions, patch application, attribution
src/firmware/ZOWI_DESKTOP_FW.hex                    # compiled artifact (committed, like the other 3 hex files)
scripts/build_firmware.sh                           # arduino-cli wrapper
```

**`ZOWI_DESKTOP_FW.ino`** — copy of `zowiLibs/code/base/ZOWI_BASE_v2.ino`
(~1100 lines, BQ GPL header preserved) with exactly three deltas:

1. `const char programID[] = "ZOWI_DESKTOP_FW";` (line 64 in the base).
2. `SCmd.addCommand("W", requestMouth);` and `SCmd.addCommand("V", requestLed);`
   in `setup()`, next to the other request commands (lines ~131-135).
3. The `requestMouth()` / `requestLed()` handlers shown above.

Everything else is untouched: all existing commands (`S/L/T/M/H/K/C/G/R/E/D/N/B/I`),
pin map, EEPROM layout, button behaviour, legacy line responses.

**`scripts/build_firmware.sh`** — behaviour:

- Resolve zowiLibs: `$1` → `$ZOWILIBS_PATH` → `~/zowiLibs` (same precedence as
  `sync_firmware_from_zowiLibs.sh`); fail with clone instructions if missing.
- **Patch guard:** verify `$ZOWILIBS_PATH/arduinolibs/Zowi/Zowi.h` contains
  `getMouth`; if not, fail with "apply src/firmware/patches/zowiLibs_read_mouth.patch first".
- Require `arduino-cli` on PATH (and the `arduino:avr` core installed; print
  `arduino-cli core install arduino:avr` if missing).
- Compile:

  ```bash
  arduino-cli compile \
      --fqbn "${FQBN:-arduino:avr:nano}" \
      --libraries "$ZOWILIBS_PATH/arduinolibs" \
      --output-dir "$BUILD_DIR" \
      src/firmware/ZOWI_DESKTOP_FW
  ```

  with `FQBN` overridable by env/flag so `arduino:avr:nano:cpu=atmega328old`
  can be tried without editing the script.
- Copy the resulting `.hex` to `src/firmware/ZOWI_DESKTOP_FW.hex` and
  normalise CRLF→LF (same hygiene as the sync script).
- The script must **not** touch the other three hex files (those come from
  `sync_firmware_from_zowiLibs.sh`).

## Phase 3 — Core (Qt-free)

| File | Change |
|---|---|
| `src/core/include/zowi/protocol.h` | Add to the `Command` enum: `GetMouth = 'W'`, `GetLed = 'V'` (query section). |
| `src/core/include/zowi/robot_commands.h` / `.cpp` | `std::string commandRequestMouth()` → `"W\r"`; `std::string commandReadLed(int row, int col)` → `"V <row> <col>\r"`. |
| `src/core/tests/test_robot_commands.cpp` | Builder tests for both commands. |
| `src/core/tests/test_message_parser.cpp` | Feed tests for `&&W <30bits>%%` and `&&V 1%%` (cmd char + value). |

**No parser change is needed:** `MessageParser` already parses any
`&&<cmd>[ <value>]%%` frame generically into `RobotMessage` — `W`/`V` arrive
as `cmd='W'/'V'` with the bits/digit as `value`. `robot_state.h` is left
alone (the mouth report is consumed on demand by the editor/CLI, not folded
into identity state).

## Phase 4 — GUI

| File | Change |
|---|---|
| `src/gui/controllers/RobotController.h/.cpp` | `Q_INVOKABLE void requestMouth()` (sends `W\r` when connected); new signal `mouthReported(QString bits)`; handle `cmd=='W'` in `parseIncoming` and emit. (Optional, symmetric: `requestLed(row,col)` + `ledReported(int,int,bool)` — only if a consumer needs it; the editor does not.) |
| `src/views/screens/MouthEditorScreen.qml` | On `Component.onCompleted`: if `Robot.connected && Robot.appId === "ZOWI_DESKTOP_FW"` → `Robot.requestMouth()`. A `Connections` block receives `mouthReported(bits)` and preloads the grid: cell `i` ← bit `29-i` (the inverse of `sendGrid()`), only while the screen is alive. New footer/side button **"Load current mouth"** (`load_mouth`) re-sends `W` on demand; disabled/hidden when the robot is not running the Desktop firmware. Empty-grid start is kept as the fallback (not connected, base firmware, no reply). |
| `src/views/screens/SettingsScreen.qml` | New option `desktop_fw`: "Install ZowiDesktop firmware" → `Robot.restoreFirmware(Config.get("desktop_firmware_path"))` (conn-gated, same flow/signals as `restore`). **Gate fix:** `isBaseFirmware()` (line 150) must also accept `appId === "ZOWI_DESKTOP_FW"` — otherwise calibration gets disabled on Desktop-firmware robots. The factory `restore` option stays enabled when running `ZOWI_DESKTOP_FW` (it is not the factory firmware), i.e. its `enabledWhen` remains `appId !== "ZOWI_BASE_v2"`; introduce a helper like `isDesktopCompatibleFirmware()` for the calibration/feature gates instead of overloading `isBaseFirmware()`. |
| `app.qrc` | `<file alias="ZOWI_DESKTOP_FW.hex">src/firmware/ZOWI_DESKTOP_FW.hex</file>` under the `/firmware` prefix. |
| `src/config.json` | New key `"desktop_firmware_path": "qrc:/firmware/ZOWI_DESKTOP_FW.hex"` (next to `factory_firmware_path`). |
| `i18n/zowi_{es_ES,en_US,ca_ES,fr_FR,bg_BG}.json` | New keys: SettingsScreen (`desktop_fw`, `desktop_fw_desc`, success/failure messages if not reusable) and MouthEditorScreen (`load_mouth`, plus `load_mouth_unsupported`/status copy if needed). |

## Phase 5 — CLI

| File | Change |
|---|---|
| `src/cli/cli_state.h/.cpp` | `const char *const kDesktopFirmwarePath = "src/firmware/ZOWI_DESKTOP_FW.hex";` (next to the existing three). |
| `src/cli/main.cpp` | New `desktop` subcommand: "Install the ZowiDesktop firmware on the paired Zowi robot", `--firmware/-f` option defaulting to `kDesktopFirmwarePath` — mirror of `alarm`/`adivinawi`. |
| `src/cli` shell (`cli_commands.cpp` / `ZOWI_CLI_SHELL.md`) | New shell commands: `readmouth` (send `W\r`, print the `&&W ...%%` reply and a small ASCII rendering of the 5×6 grid) and `readled <row> <col>` (send `V r c\r`, print `0`/`1`). Both are read-only: no `&&A%%`/`&&F%%` cycle to wait for — just send and await the framed reply. |

## Phase 6 — Docs and verification scripts

| File | Change |
|---|---|
| `docs/firmware/PROTOCOL.md` | Document `W` and `V` (command + response rows), noting they exist **only in `ZOWI_DESKTOP_FW`**, not in the stock firmwares. |
| `docs/project/FIRMWARE_HOWTO.md` | ✅ **Done (with this document):** the former "Pending Firmware Extension" section was replaced by a summary that points here, and `ZOWI_DESKTOP_FW.hex` was added to the "Firmware Format" list. |
| `docs/project/ZOWILIBS.md` | Firmware table: new row for `ZOWI_DESKTOP_FW.hex` — **built in ZowiDesktop** (`scripts/build_firmware.sh`), *not* copied by `sync_firmware_from_zowiLibs.sh`; its sources live in `src/firmware/ZOWI_DESKTOP_FW/`. Protocol table: `W`/`V` rows. Note the patch dependency. |
| `docs/project/ZOWI_CLI_SHELL.md` | `readmouth` / `readled` entries in the shell command table. |
| `docs/project/screens/SCREEN_MOUTH_EDITOR.md` | Document the preload + "Load current mouth" button and the appId gate. |
| `scripts/verify_arduino_mirrors.sh` | Also scan `src/firmware/ZOWI_DESKTOP_FW/ZOWI_DESKTOP_FW.ino`: its `addCommand` letters (base 14 + `W` + `V`) must be covered by the `Command` enum. (Today the script only fails on firmware letters missing from the enum, so adding `W`/`V` to the enum alone would not break it — but the new sketch should be covered explicitly.) |
| `CHANGELOG.md` | Entry: new firmware, `W`/`V` protocol commands, editor preload. |
| `AGENTS.md` | Mention `scripts/build_firmware.sh` + the zowiLibs patch dependency in the build section. |

## File summary

**New:**

```text
src/firmware/patches/zowiLibs_read_mouth.patch
src/firmware/ZOWI_DESKTOP_FW/ZOWI_DESKTOP_FW.ino
src/firmware/ZOWI_DESKTOP_FW/README.md
src/firmware/ZOWI_DESKTOP_FW.hex                    # generated by build_firmware.sh, then committed
scripts/build_firmware.sh
docs/firmware/FW_DESKTOP.md                         # this document
```

**Modified:**

```text
src/core/include/zowi/protocol.h                    # GetMouth='W', GetLed='V'
src/core/include/zowi/robot_commands.h              # commandRequestMouth, commandReadLed
src/core/src/robot_commands.cpp
src/core/tests/test_robot_commands.cpp
src/core/tests/test_message_parser.cpp
src/gui/controllers/RobotController.h               # requestMouth(), mouthReported()
src/gui/controllers/RobotController.cpp
src/views/screens/MouthEditorScreen.qml             # preload + load button
src/views/screens/SettingsScreen.qml                # install option + firmware gates
app.qrc                                             # bundle the hex
src/config.json                                     # desktop_firmware_path
i18n/zowi_es_ES.json  (and en_US, ca_ES, fr_FR, bg_BG)
src/cli/cli_state.h / cli_state.cpp                 # kDesktopFirmwarePath
src/cli/main.cpp                                    # desktop subcommand
src/cli/cli_commands.cpp                            # shell: readmouth, readled
docs/firmware/PROTOCOL.md
docs/project/FIRMWARE_HOWTO.md                      # ✅ done (summary section + hex list)
docs/project/INDEX.md                               # ✅ done (quick-start link to this doc)
docs/project/ZOWILIBS.md
docs/project/ZOWI_CLI_SHELL.md
docs/project/screens/SCREEN_MOUTH_EDITOR.md
scripts/verify_arduino_mirrors.sh
CHANGELOG.md
AGENTS.md
```

**External (zowiLibs repo, via the patch — applied and pushed by the
maintainer):** `arduinolibs/ZowiSerialCommand/ZowiSerialCommand.h`,
`arduinolibs/LedMatrix/LedMatrix.cpp`, `arduinolibs/Zowi/Zowi.h`,
`arduinolibs/Zowi/Zowi.cpp`.

## Verification plan

1. **Core unit tests:** `cmake --build build && ctest --test-dir build
   --output-on-failure` — new builder tests (`W\r`, `V 3 4\r`) and parser tests
   (`&&W <30bits>%%`, `&&V 1%%`).
2. **Firmware build:** patch a zowiLibs checkout, run
   `./scripts/build_firmware.sh`; confirm the `.hex` is produced. Also verify
   the patch guard fails cleanly against an unpatched zowiLibs.
3. **Mirror script:** `./scripts/verify_arduino_mirrors.sh` passes with the
   new sketch included.
4. **On hardware** (maintainer):
   - Flash: `zowi_cli desktop` (or the SettingsScreen option); robot reboots
     into the new firmware.
   - Identity: `status` reports appId `ZOWI_DESKTOP_FW`.
   - Round-trip: `mouth smile` → `readmouth` returns
     `000000100001010010001100000000`; feed it back to `mouth <bits>` —
     identical pattern on the matrix.
   - Single LED: `readled 1 1` … `readled 5 6` consistent with a known mouth
     (e.g. `lineMouth` row 3 all on).
   - Gesture truth-check: run a gesture (`H`) that ends on a known mouth, then
     `readmouth` — the value must reflect the gesture's final mouth (proves
     the read is the live matrix, not a host-side echo).
   - Pintabocas: open the editor with the robot connected → grid preloads;
     "Load current mouth" refreshes it; with a stock-firmware or disconnected
     robot the screen behaves exactly as today (empty grid, no button).
   - Regression: existing pad/mouths/gestures/calibration flows unchanged;
     calibration option enabled in Settings with `ZOWI_DESKTOP_FW`.
   - Bootloader note: if flashing fails, retry the build with
     `FQBN=arduino:avr:nano:cpu=atmega328old`.

## Open issues

- **FQBN / bootloader variant.** `arduino:avr:nano` vs
  `arduino:avr:nano:cpu=atmega328old` depends on the robot's Nano bootloader
  generation; must be settled on real hardware (first flash attempt) and then
  documented as the default in `build_firmware.sh`.
- **Flashing path.** The stock hex images were produced by BQ's toolchain;
  confirm `ZOWI_DESKTOP_FW.hex` flashes through both existing upload modes
  (`stk500UploadFirmware` and `zowiRawHexUploadFirmware`) — expected yes,
  since it is a standard arduino-cli Optiboot image, but it is untested until
  the first hardware run.
- **`requestLed()` reply on malformed input** (`V` with no/invalid args):
  `&&V 0%%` vs no reply at all. Plan says `&&V 0%%` (a host waiting for a
  frame should always get one); confirm during implementation.
- **Optional later step:** also register `W`/`V` in the stock
  `ZOWI_BASE_v2.ino` inside zowiLibs (out of scope here; the patch already
  prepares the libraries for it).

## Licensing

- `ZOWI_BASE_v2.ino` is © BQ, released under a GPL license; the zowiLibs
  Arduino libraries are LGPL-2.1. ZowiDesktop itself is GPL-3.0 — compatible.
- The derived sketch must **preserve the original BQ header/attribution** and
  note the ZowiDesktop modifications. `src/firmware/ZOWI_DESKTOP_FW/README.md`
  carries the attribution and a copy (or pointer) of the applicable licenses.
- The compiled `.hex` inherits the same terms as the firmware sources already
  shipped under `src/firmware/`.
