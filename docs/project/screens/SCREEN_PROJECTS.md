# SCREEN_PROJECT — ProjectScreen

> Educational "Discover" projects: 10 guided lessons. Each project is a
> self-contained folder under `projects/<id>/` (metadata, localized page HTML,
> localized quiz, localized strings). A **single generic `ProjectScreen.qml`**
> is pushed from the Home *Projects* page and drives any project from its
> `projectId`. It shows a learning page + external link, offers a "Run Test"
> quiz, and two of them are planned to flash an alternate firmware from the
> same window (design, see decision 3). Design derived from **ZowiAppReborn** (`ProjectViewActivity` +
> `ProjectQuizViewActivity` + `assets/projects/*.json`), adapted to the desktop
> per the decisions below.

- **Status:** ✅ **Move + Zowi's feet (bio3) + Robot form (form) + Zowi's eyes (bio1) projects implemented** (v0.8.0); 6 projects remaining (design only).
  A generic `ProjectScreen.qml` exists, backed by the Qt-free `zowi::projects` core module,
  `ProjectsController` context, `projects.qrc` resource, and a reusable `QuizComponent.qml`.
  The other 6 projects are **NOT IMPLEMENTED** — design proposal only.
- **Implemented files:**
  - Core: `src/core/include/zowi/project_model.h`, `projects_store.h/.cpp`, `projects_preferences_store.h/.cpp`
  - GUI: `src/gui/controllers/ProjectsController.h/.cpp`, `src/views/components/QuizComponent.qml`, `src/views/screens/ProjectScreen.qml`
  - Assets: `projects/index.json`, `projects/move/{project.json,page/*.html,quiz/*.json,strings/*.json}`, `projects/bio3/{project.json,page/*.html,quiz/*.json,strings/*.json}`, `projects/bio1/{project.json,page/*.html,quiz/*.json,strings/*.json}`, `images/projects/bio3_thumb.png`, `images/projects/biology_thumb.jpg`, `projects.qrc`
- **Planned files:** data folders under `projects/<id>/` for each remaining project (choreography, reprogram, helloworld, bitbloq2, adivinawi, gravity). They do **not** exist yet. No new QML screens needed.
- **Project-specific i18n** lives in each project's `strings/<locale>.json` (title, url, description) and `quiz/<locale>.json` (inline text). Shared UI strings (`test`, `learn_more`, `quiz_passed`, `quiz_failed`, `quiz_blocked`) live once in the generic `"ProjectScreen.qml"` context of `i18n/zowi_*.json`. The 10 tile titles are translated on the Home-screen context (`move_objects`,
  `choreography`, `robot_form`, `robot_eyes`, `robot_feet`, `robot_alarm`,
  `adivinawi`, `gravity`, `hello_world`, `bitbloq_sensors`).
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

1. **One generic screen for all projects.** Launching a project pushes the same
   `ProjectScreen.qml` with a `projectId` property; all content is loaded from
   that id's data folder. Navigation is **push from HomeScreen** (from the
   *Projects* page tile) and **pop** back. There is no per-project QML screen:
   adding a project is purely adding data under `projects/<id>/` + enabling its
   tile.
2. **ACHIEVEMENTS layer is deferred to the end.** The quiz only marks the
   project complete; no achievement unlock is wired yet. The feature is kept
   behind an **enable/disable switch** (a `Config` flag), so the unlock hooks
   can be added later without touching the screens.
3. **Firmware is flashed from the project's own window (design, not yet
   implemented).** Reprogram flashes `ZOWI_Alarm_v2.hex`, Adivinawi flashes
   `ZOWI_Adivinawi_v2.hex` — both from the same generic `ProjectScreen` (there
   is no per-project screen). The hex path comes from the project JSON
   (`project_hex`), so more firmware projects can be added in the future
   without code changes.
   **⚠️ Pending:** the current `ProjectScreen.qml` / `ProjectsController` do
   **not** yet expose any firmware-install UI or API; this section remains the
   target design. Until it lands, the HEX files are flashable via the CLI
   (`zowi_cli alarm` / `zowi_cli adivinawi`) and via Settings' restore flow.
4. **Project data lives in `projects.qrc`.** A new domain resource at repo
   root, alongside `views.qrc` / `app.qrc` / `images.qrc` / `i18n.qrc`, and
   appended to `GUI_QRC_FILES` in `src/gui/CMakeLists.txt`. Each project is a
   folder `projects/<id>/` (see "What a Zowi project is"). Adding a project =
   adding data files + registering them in `projects.qrc` + enabling a tile;
   no new screens or resource restructuring.

## What a Zowi project is

Every project is a small self-contained lesson stored in its own folder:

```
projects/
├── index.json                 # { "base_url": "https://…", "projects": ["move", …] }
└── <project_id>/
    ├── project.json           # metadata (mirrors the Android asset JSON)
    ├── page/<locale>.html     # localized lesson HTML (fallback en_US)
    ├── quiz/<locale>.json     # quiz with inline translated text
    └── strings/<locale>.json  # title / url / learning_description per locale
```

The `url` in each `strings/<locale>.json` is relative to the `base_url` in
`projects/index.json` (e.g. `move/es/`); `ProjectsController` joins them at
load time. Absolute URLs (`https://…`) pass through unchanged.

The metadata `project.json` mirrors the Android `com.bq.zowi.models.Project`:

- `id` — project id; drives the data folder path and the persistence keys.
- `title_key` / `description_key` / `url_key` — translation-key names used when
  a localized `strings/<locale>.json` entry is missing.
- `image_key` — detail image resource (desktop ships the original Android
  button artwork under `qrc:/images/android/*.png` / `qrc:/images/projects/*`).
- `hex_path` — optional firmware to flash from the project window (design,
  see decision 3); currently only Reprogram (Alarm) and Adivinawi carry a
  non-empty value, more can be added later.
- `achievement_id` — achievement associated with the project. **Reserved for
  the deferred ACHIEVEMENTS layer** (see decision 2): parsed and stored, but
  not acted upon until the toggle is enabled.
- `action_target` — optional in-app navigation target for the third footer
  button (e.g. `"gamepad"` in `move`); empty/absent hides the button. Mapped
  to a concrete screen in `main.qml`; the localized label is `action_label` in
  `strings/<locale>.json` (fallback: the raw target).

Quiz and UI strings are **not** keys anymore: `quiz/<locale>.json` holds the
questions with inline translated text, and `strings/<locale>.json` holds
`title` / `url` / `learning_description` per locale.

### The 10 projects → data

| # | Android id | Desktop i18n key | Data folder | Tile image | Detail image (`image_key`) | Achievement | Firmware |
|---|---|---|--------------|------------|---------------------------|-------------|----------|
| 01 | `move` | `move_objects` | `projects/move/` | `move_button.png` | `move_thumb.png` | `flapping` | — |
| 02 | `choreography` | `choreography` | `projects/choreography/` | `choreography_button.png` | — | `swing` | — |
| 03 | `form` | `robot_form` | `projects/form/` | `robot_form_button.png` | `form_thumb.jpg` | `confused` | — |
| 04 | `bio1` | `robot_eyes` | `projects/bio1/` | `eyes_button.png` | `biology_thumb.jpg` | `wave` | — |
| 05 | `bio3` | `robot_feet` | `projects/bio3/` | `feet_button.png` | `bio3_thumb.png` | `jitter` | — |
| 06 | `reprogram` | `robot_alarm` | `projects/reprogram/` | `alarm_button.png` | — | `angry` | `ZOWI_Alarm_v2.hex` |
| 07 | `helloworld` | `hello_world` | `projects/helloworld/` | `bitbloq_button.png` | — | `super_happy` | — |
| 08 | `bitbloq2` | `bitbloq_sensors` | `projects/bitbloq2/` | `bitbloq2_button.png` | — | `tip_toe` | — |
| 09 | `adivinawi` | `adivinawi` | `projects/adivinawi/` | `adivinawi_button.png` | — | `magic` | `ZOWI_Adivinawi_v2.hex` |
| 10 | `gravity` | `gravity` | `projects/gravity/` | `gravity_button.png` | — | `sleepy` | — |

Tile images live under `qrc:/images/android/`. Detail images live under
`qrc:/images/projects/` and are only assigned once a project's data folder
exists (the `image_key` is set in its `project.json` — currently `move`, `bio3`,
`form` and `bio1` are implemented, the rest are "—" until they land). The Android
counterpart artwork (`qrc:/images/android/project_*.png`) is already bundled
and can be reused. Achievements are **reserved for the deferred ACHIEVEMENTS
layer** (see decision 2).


**Firmware (design, not yet implemented in `ProjectScreen`):** both HEX files are
already bundled and flashable on desktop (`src/firmware/`, STK500v1 over BT/USB;
the CLI exposes `zowi_cli alarm` / `zowi_cli adivinawi`). Wiring an in-screen
"Install" button into the generic `ProjectScreen` is pending — see decision 3.

## Navigation

```
HomeScreen (Projects page, tile "XXX") ──projectRequested("xxx")──▶ ProjectScreen {projectId}
                                                                   │ backClicked → pop
                                                                   └─ runTest → quiz (in-screen)
```

- **Push:** Home *Projects* tiles emit a single `projectRequested(projectId)`
  signal; `main.qml` pushes `ProjectScreen.qml` with that id.
- **Pop:** the `ScreenTemplate` back button pops the stack to Home. (A future
  firmware install/failure dialog would also return to the project window —
  see decision 3.)
- Achievements toasts are **not** part of this cycle yet (deferred layer).

## ProjectScreen.qml — contents

`ProjectScreen.qml` is a `ScreenTemplate` subclass showing, for its `projectId`:

- Title, learning description and project image.
- **Done icon** — `project_done_icon` / `project_not_done_outline_icon` driven by
  `<id>_project_completeness`.
- **Footer buttons** (in order):
  1. **Learn more** (`learn_more`) — opens `project_url` in the system browser.
  2. **Questions** (`test` — "Preguntas" / "Questions" / …) — the project quiz,
     presented in the same screen via the reusable `QuizComponent` (question /
     progress / result); disabled and showing `mm:ss` countdown while a quiz
     blockade is pending (`<id>_project_quiz_blockade`).
     **Note:** The blockade is *documented and configurable* via `ProjectsPreferencesStore`
     (`blockade_duration_ms`, default 10 min), but the `mm:ss` countdown UI is not
     yet displayed in `ProjectScreen` (see [PROJECTS_HOWTO.md](../PROJECTS_HOWTO.md)).
  3. **Action button** (optional, per project) — visible only when
     `project.json` sets `action_target`; label = `action_label` from
     `strings/<locale>.json`. Emits `actionRequested(target)`; `main.qml` pops
     back to Home and pushes the destination (e.g. `"gamepad"` → PadScreen,
     via the shared `pushGamepad()` helper). `move` ships with
     `"gamepad"`; the other projects have no action button yet.
- **Install firmware** — **design, not yet implemented** (see decision 3).
  Intended only in projects with `project_hex != ""` (Reprogram → Alarm,
  Adivinawi → Adivinawi); conn-gated, 50 % battery check →
  `Robot.restoreFirmware(hex)`. The current generic `ProjectScreen` does not
  yet expose it.
- **Result handling** — all correct → `<id>_project_completeness = true` + done
  icon; wrong answer → blockade + failure feedback.
  No achievement dialog until the toggle in decision 2 is enabled.

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `projectRequested(projectId)` | Home *Projects* tiles | `main.qml`: push→ `ProjectScreen.qml` |
| `linkClicked(url)` | project link | external browser |
| `actionRequested(target)` | footer action button (only with `action_target`) | `main.qml`: pop→ Home, push→ destination (`"gamepad"` → `pushGamepad()`) |
| `installFirmwareClicked(hex)` | Install button (design only, not yet implemented) | `Robot.restoreFirmware(hex)` |
| `backClicked()` | ScreenTemplate back button | pop |
| `quizFinished(bool)` / `quizBlocked(int)` | `QuizComponent` (in-screen) | mark completeness / blockade |

## QML context used

- `Projects` (context object, backed by a Qt-free `projects` core module):
  `getProject(id)`, `loadHtml(id, locale)`, `isCompleted(id)`, `setCompleted(id)`,
  `isQuizBlocked(id)`, `blockQuiz(id)`, `getBlockadeRemainingMs(id)`,
  `getBlockadeDurationMs()`, `setBlockadeDurationMs(ms)`,
  `isAchievementsEnabled()`, `setAchievementsEnabled(bool)`,
  `isQuizEnabled()`, `setQuizEnabled(bool)` — implemented in `ProjectsController`.
  The `achievement` field is returned but currently ignored.
- `Translator` (via `tr()`), for the screen title and shared UI strings.
- `Config.get(...)` — theme colors (e.g. `color_primary`, `color_warning`,
  `color_accent`).

**Not used today (deferred firmware design, see decision 3):** `Robot`
(`connected`, `appId`, battery gating, `restoreFirmware(path)`, firmware
signals) and `Session` (`loadActiveZowiName()`). They would only come into play
once the in-screen "Install firmware" flow is implemented.

## Commands sent

- None as raw strings; the robot is not involved in the current
  `ProjectScreen` (the quiz is local logic).
- In the deferred firmware design, flashing would go through the existing
  STK500v1 backend (`Robot.restoreFirmware`), identical to Settings' restore
  flow (see decision 3).

## projects.qrc

`projects.qrc` at the repo root following the split-by-domain resource
convention, mirroring `projects/index.json` and each project folder:

```xml
<RCC>
    <qresource prefix="/projects">
        <file alias="index.json">projects/index.json</file>
        <file alias="move/project.json">projects/move/project.json</file>
        <file alias="move/page/en_US.html">projects/move/page/en_US.html</file>
        <file alias="move/quiz/en_US.json">projects/move/quiz/en_US.json</file>
        <file alias="move/strings/en_US.json">projects/move/strings/en_US.json</file>
        <!-- … one alias per file, per project … -->
    </qresource>
</RCC>
```

- Added to `GUI_QRC_FILES` in `src/gui/CMakeLists.txt`.
- Source JSON/HTML files under `projects/` (next to the other top-level resource
  trees). Debug/hot-reload loads them from disk; release loads them from the
  resource, exactly like QML and config today.
- Future projects: drop a data folder in `projects/`, add `<file>` entries in
  `projects.qrc`, register the id in `projects/index.json`, enable a tile.

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

- **Per project (in `projects/<id>/`)**: `quiz/<locale>.json` (inline question /
  answer text) and `strings/<locale>.json` (`title`, `url`,
  `learning_description`, optional `action_label`). Loaded per locale with
  `en_US` fallback. The `url` is
  joined with the global `base_url` from `projects/index.json` (absolute
  `https://…` URLs pass through).
- **Shared UI (in `i18n/zowi_*.json`)**: one `"ProjectScreen.qml"` context with
  `test`, `learn_more`, `quiz_passed`, `quiz_failed`, `quiz_blocked`. The
  reusable `QuizComponent` uses its own context `"QuizComponent.qml"` with keys:
  `correct`, `incorrect`, `quiz_blocked` (the legacy `run_test` key is unused).
- The tile titles live under the Home context (`move_objects`,
  `choreography`, …) — unchanged.

## Open questions (for review)

- Run Test as an in-screen mode vs a shared pop-up quiz component — **decided:
  reusable `QuizComponent` used inline in the single generic `ProjectScreen`**.
- ACHIEVEMENTS toggle: currently implemented in `ProjectsPreferencesStore`
  (`isAchievementsEnabled`/`setAchievementsEnabled`), exposed via `Projects`
  context. Not yet wired to UI or CLI.
- Where exactly the project JSON source files live (`projects/` vs
  `src/projects/`) — implemented at `projects/` (repo root).
- Quiz blockade countdown UI: documented and configurable via
  `ProjectsPreferencesStore.blockade_duration_ms`, but not yet displayed in
  `ProjectScreen` (shows a message bar instead). To be completed in a
  follow-up.