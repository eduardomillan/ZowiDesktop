# SCREEN_PROJECTS — ProjectXXXScreen family

> Educational "Discover" projects: 10 guided lessons. Each project ships as its
> own `ProjectXXXScreen.qml` pushed from the Home *Projects* page, shows a
> learning description + external link, offers a "Run Test" quiz, and two of
> them flash an alternate firmware from the project's own window. Design
> derived from **ZowiAppReborn** (`ProjectViewActivity` +
> `ProjectQuizViewActivity` + `assets/projects/*.json`), adapted to the desktop
> per the decisions below.

- **Status:** ✅ **Move project implemented** (v0.8.0); 9 projects remaining (design only).
  `ProjectMoveScreen.qml` exists, backed by the Qt-free `zowi::projects` core module,
  `ProjectsController` context, `projects.qrc` resource, and a reusable `QuizComponent.qml`.
  The other 9 project screens are **NOT IMPLEMENTED** — design proposal only.
- **Implemented files:**
  - Core: `src/core/include/zowi/project_model.h`, `projects_store.h/.cpp`, `projects_preferences_store.h/.cpp`
  - GUI: `src/gui/controllers/ProjectsController.h/.cpp`, `src/views/components/QuizComponent.qml`, `src/views/screens/ProjectMoveScreen.qml`
  - Assets: `projects/move.json`, `projects.qrc`, i18n keys in all locales under `"ProjectMoveScreen.qml"` and `"QuizComponent.qml"` contexts
- **Planned files:** one `src/views/screens/ProjectXXXScreen.qml` per remaining project (choreography, form, bio1, bio3, reprogram, helloworld, bitbloq2, adivinawi, gravity). They do **not** exist yet.
- **Planned i18n contexts:** `"ProjectXXXScreen.qml"` (one per screen). The 10
  tile titles are already translated on the Home-screen context (`move_objects`,
  `choreography`, `robot_form`, `robot_eyes`, `robot_feet`, `robot_alarm`,
  `adivinawi`, `gravity`, `hello_world`, `bitbloq_sensors`); descriptions, links
  and quiz strings for Move are now translated.
- **Projects data:** all project JSON files live in a dedicated
  `projects.qrc` (new Qt resource), where future projects are appended. Debug
  builds get a filesystem fallback, mirroring how `src/config.json` is both
  `:/src/config.json` and the workspace file.
- **Source of truth (Android):** ZowiAppReborn repo —
  `app/src/main/assets/projects/01_project_mueve.json` …
  `10_project_gravity.json`, `app/src/main/java/com/bq/zowi/views/interactive/projects/`
  and `docs/project/VIEWS.md` (§ProjectViewActivity / ProjectQuizViewActivity).
- **Desktop roadmap:** [docs/project/PLANNING.md](../PLANNING.md) M8 (Basic
  projects: move objects, robot form, Zowi eyes, Zowi feet, gravity) and M9
  (Advanced: choreography, alarm, fortune-telling, Bitbloq I/II).

## Design decisions (agreed, subject to review)

1. **One screen per project.** Launching a project opens
   `ProjectXXXScreen.qml`, dedicated to that project's id. Navigation is
   **push from HomeScreen** (from the *Projects* page tile) and **pop** back.
   There is no generic "project detail" screen: each project is its own QML
   subclass of `ScreenTemplate`.
2. **ACHIEVEMENTS layer is deferred to the end.** The quiz only marks the
   project complete; no achievement unlock is wired yet. The feature is kept
   behind an **enable/disable switch** (a `Config` flag), so the unlock hooks
   can be added later without touching the screens.
3. **Firmware is flashed from the project's own window.** Reprogram flashes
   `ZOWI_Alarm_v2.hex` from `ProjectReprogramScreen`, Adivinawi flashes
   `ZOWI_Adivinawi_v2.hex` from `ProjectAdivinawiScreen`. The hex path comes
   from the project JSON (`project_hex`), so more firmware projects can be
   added in the future without code changes.
4. **Project JSONs live in `projects.qrc`.** A new domain resource at repo
   root, alongside `views.qrc` / `app.qrc` / `images.qrc` / `i18n.qrc`, and
   appended to `GUI_QRC_FILES` in `src/gui/CMakeLists.txt`. Adding a project =
   adding a `.json` + a tile + a screen; no resource restructuring.

## What a Zowi project is

Every project is a small self-contained lesson defined in an asset JSON. In the
Android app this maps 1:1 to the model `com.bq.zowi.models.Project`, which the
desktop mirrors as a Qt-free core type:

- `id` — project id; names the QML screen (`Project<Id>Screen.qml`), the JSON
  file and the persistence keys.
- `title_resource_id` / `learning_description_resource_id` — i18n keys
  (resolved through the TranslationEngine, never literal strings).
- `image_resource_id` — tile/detail image (desktop already ships the original
  Android button artwork under `qrc:/images/android/*.png`).
- `project_url_resource_id` — external link opened in the system browser.
- `test` — quiz content: 2 questions x 3 answers, one correct each.
- `achievement` — achievement id associated with the project. **Reserved for
  the deferred ACHIEVEMENTS layer** (see decision 2): parsed and stored, but
  not acted upon until the toggle is enabled.
- `project_hex` — optional firmware to flash from the project's own window;
  currently only Reprogram (Alarm) and Adivinawi carry a non-empty value, more
  can be added later.

### The 10 projects → screens

| # | Android id | JSON file | Desktop i18n key | Screen | Firmware |
|---|---|---|---|---|---|
| 01 | `move` | `01_project_mueve.json` | `move_objects` | `ProjectMoveScreen.qml` | — |
| 02 | `choreography` | `02_project_choreography.json` | `choreography` | `ProjectChoreographyScreen.qml` | — |
| 03 | `form` | `03_project_forma.json` | `robot_form` | `ProjectFormScreen.qml` | — |
| 04 | `bio1` | `04_project_bio1.json` | `robot_eyes` | `ProjectBio1Screen.qml` | — |
| 05 | `bio3` | `05_project_bio3.json` | `robot_feet` | `ProjectBio3Screen.qml` | — |
| 06 | `reprogram` | `06_project_reprogram.json` | `robot_alarm` | `ProjectReprogramScreen.qml` | `ZOWI_Alarm_v2.hex` |
| 07 | `helloworld` | `07_project_helloworld.json` | `hello_world` | `ProjectHelloWorldScreen.qml` | — |
| 08 | `bitbloq2` | `08_project_bitbloq2.json` | `bitbloq_sensors` | `ProjectBitbloq2Screen.qml` | — |
| 09 | `adivinawi` | `09_project_adivinawi.json` | `adivinawi` | `ProjectAdivinawiScreen.qml` | `ZOWI_Adivinawi_v2.hex` |
| 10 | `gravity` | `10_project_gravity.json` | `gravity` | `ProjectGravityScreen.qml` | — |

Both firmware HEX files are already bundled and flashable on desktop
(`src/firmware/`, STK500v1 over BT/USB; the CLI exposes `zowi_cli alarm` /
`zowi_cli adivinawi` and the GUI has `Robot.restoreFirmware(path)`).

## Navigation

```
HomeScreen (Projects page, tile "XXX") ──projectXXXClicked()──▶ ProjectXXXScreen
                                                                │ backClicked → pop
                                                                └─ runTest → quiz (in-screen)
```

- **Push:** Home *Projects* tiles become clickable and emit a per-project
  signal; `main.qml` pushes the matching `ProjectXXXScreen.qml`.
- **Pop:** the `ScreenTemplate` back button pops the stack to Home; the firmware
  install/failure dialogs also return to the project window (not Home).
- Achievements toasts are **not** part of this cycle yet (deferred layer).

## ProjectXXXScreen.qml — contents

Each project screen is a `ScreenTemplate` subclass showing, for its project:

- Title, learning description and project image.
- **Done icon** — `project_done_icon` / `project_not_done_icon` driven by
  `<id>_project_completeness`.
- **Project link** — opens `project_url` in the system browser.
- **Install firmware** — **only in projects with `project_hex != ""`**
  (Reprogram → Alarm, Adivinawi → Adivinawi; future firmware projects reuse
  it). Conn-gated, 50 % battery check → `Robot.restoreFirmware(hex)` with the
  existing progress/low-battery dialogs (mirrors the Settings restore flow).
  Triggered and resolved from the project's own window.
- **Run Test** — the project quiz, presented in the same screen via the reusable
  `QuizComponent` (question / progress / result); disabled and showing `mm:ss`
  countdown while a quiz blockade is pending (`<id>_project_quiz_blockade`).
  **Note:** The blockade is *documented and configurable* via `ProjectsPreferencesStore`
  (`blockade_duration_ms`, default 10 min), but the countdown UI is not yet wired
  for the Move project (see [PROJECTS_HOWTO.md](../PROJECTS_HOWTO.md)).
- **Result handling** — all correct → `<id>_project_completeness = true` + done
  icon; wrong answer → blockade + failure feedback.
  No achievement dialog until the toggle in decision 2 is enabled.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `projectMoveClicked()` … `projectGravityClicked()` | Home *Projects* tiles | `main.qml`: push→ `ProjectXXXScreen` |
| `linkClicked(url)` | project link | external browser |
| `installFirmwareClicked(hex)` | Install button (only if `project_hex != ""`) | `Robot.restoreFirmware(hex)` |
| `backClicked()` | ScreenTemplate back button | pop |
| `quizFinished(bool)` / `quizBlocked(int)` | `QuizComponent` (in-screen) | mark completeness / blockade |

## QML context used

- `Robot`: `connected`, `appId`, battery gating (`battery >= 50`), firmware
  signals `onFirmwareRestoreStarted/Progress/Finished/BatteryLow`,
  `restoreFirmware(path)`.
- `Config.get(...)`: theme colors; `project_*` asset paths; the
  **achievements enabled/disabled flag** (decision 2).
- `Session`: `loadActiveZowiName()` (firmware dialogs reference the name).
- `Projects` (context object, backed by a Qt-free `projects` core module):
  `getProject(id)`, `isCompleted(id)`, `isQuizBlocked(id)`, `blockQuiz(id)`,
  `setCompleted(id)`, `getBlockadeDurationMs()`, `setBlockadeDurationMs(ms)`,
  `isAchievementsEnabled()`, `setAchievementsEnabled(bool)`,
  `isQuizEnabled()`, `setQuizEnabled(bool)` — implemented in `ProjectsController`.
  The `achievement` field is returned but currently ignored.
- `Translator` (via `tr()`).

## Commands sent

- None as raw strings. Firmware flashing goes through the existing STK500v1
  backend (`Robot.restoreFirmware`), identical to Settings' restore flow.
- The quiz is local logic; the robot is not involved.

## projects.qrc

New file `projects.qrc` at the repo root following the split-by-domain resource
convention:

```xml
<RCC>
    <qresource prefix="/projects">
        <file alias="move.json">projects/move.json</file>
        <file alias="choreography.json">projects/choreography.json</file>
        <!-- … one entry per project … -->
        <file alias="gravity.json">projects/gravity.json</file>
    </qresource>
</RCC>
```

- Added to `GUI_QRC_FILES` in `src/gui/CMakeLists.txt`.
- Source JSON files under `projects/` (next to the other top-level resource
  trees). Debug/hot-reload loads them from disk; release loads them from the
  resource, exactly like QML and config today.
- Future projects: drop a `.json` in `projects/`, add an `<file>` entry,
  register a tile in HomeScreen and add the matching `ProjectXXXScreen.qml`.

## Persistence

Two keys per project, aligned with the Android `SharedPreferences` names and
the session/config-store conventions (see AGENTS.md):

- `<id>_project_completeness` — `true` once the quiz is passed.
- `<id>_project_quiz_blockade` — epoch millis of the last wrong answer;
  ≤ `now` means not blocked, `> now` yields a `mm:ss` countdown.

"Forget playing history" (Settings) resets all `*_project_*` keys (not yet wired).

Project preferences (blockade duration, achievements toggle, quiz enabled) are
stored in a separate `projects_preferences.json` file via `ProjectsPreferencesStore`.

## i18n

New keys per project (`<prefix>_title`, `_learning_description`, `_url`,
`_question_1/2`, `_question_1/2_answer_1..3`) in all desktop locales,
grouped under each `"ProjectXXXScreen.qml"` context. The Android strings
(`strings.xml`) and the desktop `i18n/zowi_*.json` naming differ
(`project_move_title` vs `move_objects`) — the tile titles already exist
under the Home context; the per-screen keys for Move are now implemented.

The reusable `QuizComponent` uses its own context `"QuizComponent.qml"` with
keys: `run_test`, `correct`, `incorrect`, `quiz_blocked`.

## Open questions (for review)

- Run Test as an in-screen mode vs a shared pop-up quiz component — **decided:
  reusable `QuizComponent` used inline in each project screen**.
- ACHIEVEMENTS toggle: currently implemented in `ProjectsPreferencesStore`
  (`isAchievementsEnabled`/`setAchievementsEnabled`), exposed via `Projects`
  context. Not yet wired to UI or CLI.
- Where exactly the project JSON source files live (`projects/` vs
  `src/projects/`) — implemented at `projects/` (repo root).
- Quiz blockade countdown UI: documented and configurable via
  `ProjectsPreferencesStore.blockade_duration_ms`, but not yet displayed in
  `ProjectMoveScreen` (shows a message bar instead). To be completed in a
  follow-up.