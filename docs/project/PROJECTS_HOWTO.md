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
- Optional firmware flashing (for Reprogram and Adivinawi projects)

The first implemented project is **Move** (id: `move`, Home tile: `move_objects`).

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        GUI (Qt/QML)                             │
│  ┌─────────────┐  ┌─────────────────┐  ┌────────────────────┐  │
│  │ HomeScreen  │  │ ProjectMoveScreen│  │   QuizComponent    │  │
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
| `src/core/include/zowi/project_model.h` | `Project`, `ProjectQuestion`, `ProjectAnswer` structs + `parseProjectJson()` |
| `src/core/include/zowi/projects_store.h` | `ProjectsStore` class — loads/parses JSON, provides `getProject(id)`, `getAllProjects()` |
| `src/core/include/zowi/projects_preferences_store.h` | `ProjectsPreferencesStore` — configurable params (blockade, achievements, quiz) |
| `src/core/src/projects_store.cpp` | Implementation of `ProjectsStore` |
| `src/core/src/projects_preferences_store.cpp` | Implementation of `ProjectsPreferencesStore` |

### ProjectsStore

- **Constructor**: `ProjectsStore()` — default loader reads `projects/move.json` from filesystem (dev) or `:/projects/` (release).
- **Resource loading**: Call `setResourceBasePath(":/projects")` to point at Qt resource prefix.
- **Custom loader**: Call `setProjectsLoader(loader)` for platform-specific loading (e.g., Android assets).
- **loadAll()**: Parses JSON array or single object, populates `m_projects` vector.
- **getProject(id)**: Returns `std::optional<Project>`.
- **getAllProjects()**: Returns all loaded projects.

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

- Uses `TranslatorController` to resolve i18n keys at runtime (context = `Project<Id>Screen.qml`).
- Uses `SessionController` generic `saveString/getString` for persistence keys:
  - `<id>_project_completeness` → `"true"` / `""`
  - `<id>_project_quiz_blockade` → epoch millis as string
- Emits `projectsChanged()` signal when store or prefs change.

### ProjectMoveScreen (`src/views/screens/ProjectMoveScreen.qml`)

- Inherits `ScreenTemplate` with `showBackButton: true`
- Uses `Projects.getProject("move")` for all content
- Embeds `QuizComponent` inline with `projectId: "move"` and `questions: project.questions`
- Handles `QuizComponent.finished` / `blocked` signals to show `MessageBar` feedback
- External link via `Qt.openUrlExternally(project.url)`

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

1. **Create JSON** in `projects/<id>.json`:

```json
{
  "id": "choreography",
  "title_key": "title",
  "description_key": "learning_description",
  "image_key": "image",
  "url_key": "url",
  "questions": [
    {
      "text_key": "question_1",
      "answers": [
        { "text_key": "question_1_answer_1", "correct": false },
        { "text_key": "question_1_answer_2", "correct": true },
        { "text_key": "question_1_answer_3", "correct": false }
      ]
    },
    { ... question 2 ... }
  ],
  "hex_path": "",
  "achievement_id": "choreography_achievement"
}
```

2. **Add to `projects.qrc`**:

```xml
<file alias="choreography.json">projects/choreography.json</file>
```

3. **Add i18n keys** in all 5 `i18n/zowi_*.json` under `"ProjectChoreographyScreen.qml"` context:
   - `title`, `learning_description`, `url`, `project_link`, `run_test`, `quiz_passed`, `quiz_failed`, `quiz_blocked`, `correct`, `incorrect`
   - `question_1`, `question_1_answer_1/2/3`, `question_2`, `question_2_answer_1/2/3`

4. **Create screen** `src/views/screens/ProjectChoreographyScreen.qml` (copy `ProjectMoveScreen.qml`, change `projectId` to `"choreography"`).

5. **Add to `views.qrc`** under screens section.

6. **Enable tile in `HomeScreen.qml`**:
   - In `projectsData`, set `enabled: true` for the project's entry
   - Add `signal projectChoreographyClicked()` to `FocusScope`
   - In the Flow's `MouseArea`, add handler for the tile name

7. **Wire in `main.qml`** `connectHome()`:
   ```qml
   home.projectChoreographyClicked.connect(function() {
       var screen = stack.push("qrc:/src/views/screens/ProjectChoreographyScreen.qml")
       screen.backClicked.connect(function() { stack.pop() })
   })
   ```

7. **Rebuild**: `./build.sh` or `cmake --build build`

---

## Quiz component

The `QuizComponent` is designed to be **inlined** in each `ProjectXXXScreen.qml`:

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

Each project screen uses its own context: `"Project<Id>Screen.qml"` (e.g., `"ProjectMoveScreen.qml"`).

The reusable `QuizComponent` uses `"QuizComponent.qml"`.

Keys are stored in `i18n/zowi_<locale>.json` (5 locales: `en_US`, `es_ES`, `ca_ES`, `fr_FR`, `bg_BG`).

**Required keys per project:**

| Key | Purpose |
|-----|---------|
| `title` | Screen title |
| `learning_description` | Description text |
| `url` | External link URL |
| `project_link` | Button label for link |
| `run_test` | "Run Test" button |
| `quiz_passed` | Success message |
| `quiz_failed` | Failure message (with `%1` = countdown) |
| `quiz_blocked` | Blocked message (with `%1` = countdown) |
| `correct` | Answer feedback |
| `incorrect` | Answer feedback |
| `question_1` | Q1 text |
| `question_1_answer_1/2/3` | Q1 answers |
| `question_2` | Q2 text |
| `question_2_answer_1/2/3` | Q2 answers |

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
3. Verify "Move objects" tile is enabled (colored, not greyed out)
4. Click tile → `ProjectMoveScreen` opens
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