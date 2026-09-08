# PROJECTS_HOWTO — Projects system implementation guide

## Table of contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Core module (Qt-free)](#core-module-qt-free)
- [GUI integration](#gui-integration)
- [Adding a new project](#adding-a-new-project)
- [Quiz component](#quiz-component)
- [Projects preferences](#projects-preferences)
- [i18n](#i18n)
- [Persistence](#persistence)
- [Testing](#testing)
- [CLI integration (future)](#cli-integration-future)

---

## Overview

The Projects system implements the "Discover" educational projects from ZowiAppReborn
as desktop screens. Each project is a self-contained lesson with:

- Title, description, image, and external link
- An in-screen quiz (2 questions × 3 answers)
- Persistence of completion state and quiz blockade
- Optional firmware flashing for Reprogram and Adivinawi — **designed, not yet
  implemented** in the current generic `ProjectScreen` (see decision 3 in
  SCREEN_PROJECTS.md)

The implemented projects are **Move** (id: `move`, Home tile: `move_objects`),
**Zowi's feet** (id: `bio3`, Home tile: `robot_feet`), **Robot form** (id:
`form`, Home tile: `robot_form`), **Zowi's eyes** (id: `bio1`, Home tile:
`robot_eyes`) and **Gravity** (id: `gravity`, Home tile: `gravity`, whose
`action_target: "calibration"` pushes the CalibrationScreen).


---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        GUI (Qt/QML)                             │
│  ┌─────────────┐  ┌─────────────────┐  ┌────────────────────┐  │
│  │ HomeScreen  │  │  ProjectScreen  │  │   QuizComponent    │  │
│  │ (tile push) │──▶│ (ScreenTemplate)│──▶│ (reusable inline)  │  │
│  └─────────────┘  └────────┬────────┘  └────────────────────┘  │
│                           │                                     │
│                    ┌──────▼──────┐                              │
│                    │  Projects   │                              │
│                    │ Controller  │                              │
│                    └──────┬──────┘                              │
└───────────────────────────┼─────────────────────────────────────┘
                            │
         ┌──────────────────┼──────────────────┐
         ▼                  ▼                  ▼
┌─────────────────┐ ┌───────────────┐ ┌─────────────────┐
│  ProjectsStore  │ │ProjectsPrefs  │ │  SessionStore   │
│  (core, Qt-free)│ │  (core, Qt-free)│ │  (core, Qt-free)│
│                 │ │               │ │                 │
│ - loads JSON    │ │ - blockade_ms │ │ - completeness  │
│ - parses models │ │ - achievements│ │ - quiz_blockade │
└─────────────────┘ └───────────────┘ └─────────────────┘
```

**Key design principles:**

1. **Core is Qt-free** — `ProjectsStore`, `ProjectsPreferencesStore`, `Project` model live in `src/core/` and depend only on `nlohmann/json` and STL. Usable from CLI, Android (JNI/FFI), or future WebAssembly ports.

2. **Single context property** — `Projects` (a `ProjectsController` instance) is registered in `main.cpp` and exposes all project-related QML APIs.

3. **Reusable quiz** — `QuizComponent.qml` is an inline component (not a separate screen) that handles question flow, answer validation, blockade timing, and emits `finished(bool)` / `blocked(int)`.

4. **Configurable blockade** — Quiz blockade duration (default 10 min) is stored in `ProjectsPreferencesStore` and can be changed at runtime via `Projects.setBlockadeDurationMs(ms)`.

5. **Deferred achievements** — The `achievements_enabled` preference exists but the UI layer is not wired yet.

---

## Core module (Qt-free)

### Files

| File | Purpose |
|------|---------|
| `src/core/include/zowi/project_model.h` | `Project` struct + `parseProjectJson()` |
| `src/core/include/zowi/projects_store.h` | `ProjectsStore` class — loads/parses JSON, provides `getProject(id)`, `getAllProjects()` |
| `src/core/include/zowi/projects_preferences_store.h` | `ProjectsPreferencesStore` — configurable params (blockade, achievements, quiz) |
| `src/core/src/projects_store.cpp` | Implementation of `ProjectsStore` |
| `src/core/src/projects_preferences_store.cpp` | Implementation of `ProjectsPreferencesStore` |

### ProjectsStore

- **Constructor**: `ProjectsStore()` — default loader reads `projects/index.json` from filesystem (dev) or `:/projects/` (release), then bundles every `projects/<id>/project.json` listed there into a single JSON array.
- **Resource loading**: Call `setResourceBasePath(":/projects")` to point at Qt resource prefix.
- **Custom loader**: Call `setProjectsLoader(loader)` for platform-specific loading (e.g., Android assets). The GUI's `ProjectsController` uses this and performs its own qrc/disk fallback resolution.
- **loadAll()**: Parses JSON array or single object, populates `m_projects` vector.
- **getProject(id)**: Returns `std::optional<Project>`.
- **getAllProjects()**: Returns all loaded projects.
- The `Project` model carries only metadata (`id`, translation keys, `hexPath`, `achievementId`). Quiz content is locale-specific and lives in per-project `quiz/<locale>.json` files — see "Adding a new project".

### ProjectsPreferencesStore

- **Constructor**: `ProjectsPreferencesStore(configDir = "")` — uses same config dir resolution as `SessionStore` (platform default or explicit).
- **Persistence file**: `projects_preferences.json` in the same directory as `ZowiApp.json`.
- **API**:
  - `getBlockadeDurationMs()` / `setBlockadeDurationMs(int ms)` — default 600000 (10 min)
  - `isAchievementsEnabled()` / `setAchievementsEnabled(bool)` — default false
  - `isQuizEnabled()` / `setQuizEnabled(bool)` — default true
  - `getProjectOverrides(projectId)` / `setProjectOverride(projectId, json)` — per-project future config

---

## GUI integration

### ProjectsController (`src/gui/controllers/ProjectsController.h/.cpp`)

Registered in `main.cpp` as context property `"Projects"`.

**QML API:**

```qml
// Get full project data (translated strings)
Projects.getProject("move")  // → { id, title, description, image, url, questions[], hexPath, achievementId }

// Completion & blockade
Projects.isCompleted("move")           // bool
Projects.isQuizBlocked("move")         // bool
Projects.getBlockadeRemainingMs("move") // int (ms)
Projects.blockQuiz("move", durationMs)  // void
Projects.setCompleted("move")           // void

// Preferences
Projects.getBlockadeDurationMs()        // int
Projects.setBlockadeDurationMs(ms)      // void
Projects.isAchievementsEnabled()        // bool
Projects.setAchievementsEnabled(bool)   // void
Projects.isQuizEnabled()                // bool
Projects.setQuizEnabled(bool)           // void
```

**Implementation notes:**

- `getProject(id)` loads metadata from core, then resolves project-specific strings (`title`, `url`, `description`) from `projects/<id>/strings/<locale>.json` and the quiz from `projects/<id>/quiz/<locale>.json`, with fallback to `en_US` when a locale file is missing. The `url` is joined with `base_url` read from `projects/index.json` (relative `url` values), or returned verbatim for absolute URLs. It no longer resolves quiz/strings through the Translator; shared UI strings come from the generic `"ProjectScreen.qml"` context in i18n.
- Uses `SessionController` generic `saveString/getString` for persistence keys:
  - `<id>_project_completeness` → `"true"` / `""`
  - `<id>_project_quiz_blockade` → epoch millis as string
- Emits `projectsChanged()` signal when store or prefs change.

### ProjectScreen (`src/views/screens/ProjectScreen.qml`)

- Generic screen for all learning projects. Inherits `ScreenTemplate` with `showBackButton: true`.
- Has a `projectId` property (set when pushed from `main.qml`); all content is loaded via `Projects` for that id.
- Uses `Projects.getProject(projectId)` for metadata (title, image, url, quiz questions).
- Embeds `QuizComponent` inline with `projectId` and `questions: project.questions`.
- Handles `QuizComponent.finished` / `blocked` signals to show `MessageBar` feedback.
- External link via `Qt.openUrlExternally(project.url)`.
- Is the **single generic screen for all learning projects** (it replaced the
  former per-project `ProjectXxxScreen.qml` approach). Inherits `ScreenTemplate`
  with `showBackButton: true`; adding a new project does **not** require a new
  QML screen.

### QuizComponent (`src/views/components/QuizComponent.qml`)

**Properties:**

- `projectId: string` — used for blockade persistence
- `questions: array` — each: `{ text: string, answers: [{ text, correct }, ...] }`

**Signals:**

- `finished(bool allCorrect)`
- `blocked(int remainingMs)`

**Internal logic:**

1. Shows "Run Test" button when idle
2. On click: iterates questions, shows 3 answer buttons per question
3. On answer: marks correct/incorrect, shows feedback for 1.5s
4. If all correct → `finished(true)`
5. If any wrong → calls `Projects.blockQuiz(projectId, Projects.getBlockadeDurationMs())`, emits `blocked(duration)`, `finished(false)`
6. Blockade timer ticks every second, updates `blockadeCountdown` (mm:ss)
7. While blocked: "Run Test" hidden, countdown text shown

---

## Adding a new project

Each project is a self-contained folder under `projects/`:

```
projects/
├── index.json                 # { "base_url": "...", "projects": ["move", ...] }
└── <project_id>/
    ├── project.json           # metadata (translation keys, hex path, achievement)
    ├── page/<locale>.html     # localized lesson HTML
    ├── quiz/<locale>.json     # quiz with INLINE translated text (per locale)
    └── strings/<locale>.json  # project-specific UI strings (title, url, description)
```

`projects/index.json` also carries a global `base_url`: project `url` values in the
strings files are **relative** to it (e.g. `move/es/`), and `ProjectsController`
joins them at load time (absolute URLs — containing `://` — pass through unchanged).

1. **Create metadata** in `projects/<id>/project.json`:

```json
{
  "id": "choreography",
  "title_key": "title",
  "description_key": "learning_description",
  "image_key": "qrc:/images/projects/choreography_thumb.png",
  "url_key": "url",
  "hex_path": "",
  "achievement_id": "choreography_achievement",
  "action_target": ""
}
```

`action_target` is **optional** (empty/absent = no action button): it configures
the third footer button of `ProjectScreen`, which navigates to a place inside
the app (e.g. `"gamepad"` in the `move` project). The target string is mapped
to a concrete screen in `main.qml` (`actionRequested(target)`: pop back to
Home, then push the destination); unknown targets are logged and ignored. The
button label is the per-locale `action_label` in `strings/<locale>.json`
(fallback: the raw target string).

2. **Add page HTML** in `projects/<id>/page/<locale>.html` for each supported locale (fallback to `en_US.html` when a locale is missing).

3. **Add quiz** in `projects/<id>/quiz/<locale>.json` with inline translated strings:

```json
{
  "questions": [
    {
      "text": "Question text in this locale",
      "answers": [
        { "text": "Wrong", "correct": false },
        { "text": "Right", "correct": true },
        { "text": "Wrong", "correct": false }
      ]
    }
  ]
}
```

4. **Add strings** in `projects/<id>/strings/<locale>.json`. The `url` is
   **relative** to `base_url` from `projects/index.json` (fall back to `en_US.json`
   when a locale file is missing):

```json
{ "title": "Choreography", "url": "choreography/es/", "learning_description": "...", "action_label": "..." }
```
If a project needs an external/absolute destination, put a full `https://…` URL
instead — it is left untouched. `action_label` is optional and only needed when
`project.json` sets an `action_target` (localized label of the third button).

5. **Register in `projects.qrc`**: add `index.json`, the project's `project.json`, every `page/*.html`, `quiz/*.json` and `strings/*.json`.

6. **Add project id to `projects/index.json`** `projects` array.

7. **Enable tile in `HomeScreen.qml`**: in `projectsData`, set `enabled: true` for the project's entry (the navigation already emits `projectRequested(id)` generically).

8. **Rebuild**: `./build.sh` or `cmake --build build`

No new QML screen is needed: `ProjectScreen.qml` reads all content for a given `projectId`. Shared UI strings (`test`, `learn_more`, `quiz_passed`, `quiz_failed`, `quiz_blocked`) live once in the generic `ProjectScreen.qml` context of `i18n/zowi_*.json`, not per project. The footer buttons appear in this order: **Learn more** (`learn_more`), **Questions** (`test` — "Preguntas" and translations), and the optional per-project **action button** (`action_target`/`action_label`).

---

## Quiz component

The `QuizComponent` is designed to be **inlined** in the single generic
`ProjectScreen.qml` (shared by every project):

```qml
QuizComponent {
    id: quizComponent
    projectId: "move"
    questions: project.questions
    onFinished: onQuizFinished(allCorrect)
    onBlocked: onQuizBlocked(remainingMs)
}
```

**Customization points:**

- Question/answer rendering: uses `Qt.createQmlObject` to create buttons dynamically (could be refactored to a `Repeater` + `Component` for cleaner code).
- Feedback duration: `nextTimer.interval` (default 1500ms).
- Blockade countdown: updates every second via `blockadeTimer`.

---

## Projects preferences

The `ProjectsPreferencesStore` (core) + `ProjectsController` (GUI) expose:

| Preference | Key | Default | Description |
|------------|-----|---------|-------------|
| Blockade duration | `blockade_duration_ms` | 600000 (10 min) | Quiz lockout after wrong answer |
| Achievements enabled | `achievements_enabled` | false | Deferred layer toggle |
| Quiz enabled | `quiz_enabled` | true | Global quiz on/off |
| Project overrides | `projects.<id>` | `{}` | Per-project future config |

**QML access:**

```qml
Projects.getBlockadeDurationMs()
Projects.setBlockadeDurationMs(300000)  // 5 min
Projects.isAchievementsEnabled()
Projects.setAchievementsEnabled(true)
Projects.isQuizEnabled()
Projects.setQuizEnabled(false)
```

**Persistence file**: `projects_preferences.json` (same dir as `ZowiApp.json`).

---

## i18n

There is a **single generic `ProjectScreen.qml`** shared by every project, so the
screen has **one** shared UI context — there is no per-project
`"Project<Id>Screen.qml"` context anymore.

Strings live in two places:

1. **Shared UI (in `i18n/zowi_<locale>.json`)** under the `"ProjectScreen.qml"`
   context: `test`, `learn_more`, `quiz_passed`, `quiz_failed`, `quiz_blocked`
   (for all implemented locales). The reusable `QuizComponent` uses its own
   context `"QuizComponent.qml"` with `correct`, `incorrect`, `quiz_blocked`.
2. **Per project (in `projects/<id>/` data folders)**, not as i18n keys:
   - `strings/<locale>.json` → `title`, `url`, `learning_description`.
   - `quiz/<locale>.json` → the quiz questions with inline translated text.

**Legacy note:** the old per-project model used keys like `project_link`,
`run_test`, `question_1`, `question_1_answer_1/2/3`, etc. These are obsolete and
have been replaced by the data-folder layout above (see "Adding a new project").


---

## Persistence

### SessionStore keys (per project)

| Key | Value | Description |
|-----|-------|-------------|
| `<id>_project_completeness` | `"true"` / `""` | Quiz passed |
| `<id>_project_quiz_blockade` | epoch millis (string) | Blockade expires at |

Stored in `ZowiApp.json` (same file as other session data).

### ProjectsPreferencesStore keys (global)

| Key | Value | Description |
|-----|-------|-------------|
| `blockade_duration_ms` | int | Blockade duration in ms |
| `achievements_enabled` | bool | Achievements layer toggle |
| `quiz_enabled` | bool | Global quiz on/off |
| `projects.<id>` | object | Per-project overrides |

Stored in `projects_preferences.json` (separate file).

---

## Testing

### Core tests

Add to `src/core/tests/`:

```cpp
// test_projects_store.cpp
#include <zowi/projects_store.h>
#include <zowi/project_model.h>
#include <gtest/gtest.h>

TEST(ProjectsStore, LoadMoveProject) {
    zowi::ProjectsStore store;
    store.setResourceBasePath("path/to/projects"); // or use custom loader
    store.loadAll();
    auto proj = store.getProject("move");
    ASSERT_TRUE(proj.has_value());
    EXPECT_EQ(proj->id, "move");
    EXPECT_EQ(proj->questions.size(), 2);
    EXPECT_EQ(proj->questions[0].answers.size(), 3);
}
```

Run: `ctest --test-dir build -R test_projects_store --output-on-failure`

### GUI manual test

1. Launch `ZowiDesktop`
2. Navigate to Home → Projects page (swipe right)
3. Verify the enabled project tiles ("Move objects", "Robot form", "Zowi's eyes" (bio1), "Zowi's feet" (bio3), "Gravity") are enabled (colored, not greyed out)
4. Click a tile → `ProjectScreen` opens (with the matching `projectId`)
5. Verify title, description, image, link button work
6. Click "Run Test" → answer questions
7. All correct → success message, done icon appears
8. One wrong → blockade message, "Run Test" hidden, countdown shown
9. Restart app → verify completion persists, blockade persists

---

## CLI integration (future)

The CLI (`zowi_cli`) currently has no project commands. Planned for M8:

| Command | Description |
|---------|-------------|
| `zowi_cli project list` | List all projects with completion status |
| `zowi_cli project show <id>` | Show project details |
| `zowi_cli project reset <id>` | Reset completion + blockade |
| `zowi_cli project prefs` | Show/set preferences (blockade, achievements, quiz) |

Implementation would use the core `ProjectsStore` and `ProjectsPreferencesStore` directly (no Qt), similar to how `cli_commands.cpp` uses `SessionStore` and `ConfigStore`.

**Note:** The `projects_preferences.json` file is already compatible with CLI access since it uses the same config dir resolution as `SessionStore`.