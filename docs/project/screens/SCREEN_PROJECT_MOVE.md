# SCREEN_PROJECT_MOVE — ProjectMoveScreen.qml

> Project 01 "Move" — learn to guide Zowi precisely and understand how a
> two-legged robot moves. Design derived from ZowiAppReborn's
> `01_project_mueve.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ✅ **IMPLEMENTED** (v0.8.0).
- **File:** `src/views/screens/ProjectMoveScreen.qml` (implemented).
- **i18n context:** `"ProjectMoveScreen.qml"` (implemented in all locales).
- **Project id:** `move` — Home tile `move_objects`.
- **Source JSON:** `projects/move.json` (registered in `projects.qrc`).
- **Achievement:** `flapping` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/projects/move-objects/`
- **Images:** tile `qrc:/images/android/move_button.png`; detail
  `qrc:/images/android/project_move_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents

- Title ("Move"), learning description and project image.
- Done icon — `move_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz via reusable `QuizComponent`, in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz

| Q | Answers (correct in bold) |
|---|---|
| 1 | The movements of Zowi allow it to… — "…spin in circles." / **"…walk forward and diagonally."** / "…walk backward and diagonally." |
| 2 | If I increase the walking speed of Zowi, does it always move faster? — **"No. It moves more quickly, but sometimes it does not advance faster because it slips."** / "No. If it moves too fast it can break and stop advancing." / "Yes. Zowi always advances faster if the movement speed is higher." |

All correct → `move_project_completeness = true` + done icon; wrong answer →
`move_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No achievement
dialog until the ACHIEVEMENTS toggle is enabled (see SCREEN_PROJECTS.md).

**Note:** The blockade countdown UI is documented and configurable via
`ProjectsPreferencesStore.blockade_duration_ms` (default 600000 ms), but the
`mm:ss` countdown display in the quiz is not yet wired for Move. The
`QuizComponent` handles the blockade logic and emits `blocked(remainingMs)`.

## Signals / QML context

- `backClicked()` → pop; in-screen quiz signals `quizFinished(bool)` / `quizBlocked(int)`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("move")`, `isCompleted`,
  `isQuizBlocked`, `blockQuiz`, `setCompleted`, `getBlockadeDurationMs`,
  `setBlockadeDurationMs`, `isAchievementsEnabled`, `setAchievementsEnabled`,
  `isQuizEnabled`, `setQuizEnabled`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).

## Implementation notes

- Core: `zowi::ProjectsStore` loads `:/projects/move.json` (resource) or filesystem (dev).
- GUI: `ProjectsController` wraps the core store and exposes the QML API.
- Quiz: `QuizComponent.qml` (reusable) handles question flow, answer validation,
  blockade timing, and persistence via `SessionController.saveString/getString`.
- Preferences: `ProjectsPreferencesStore` stores `blockade_duration_ms`,
  `achievements_enabled`, `quiz_enabled` in `projects_preferences.json`.
- i18n: All strings in all locales under `"ProjectMoveScreen.qml"` and
  `"QuizComponent.qml"` contexts.