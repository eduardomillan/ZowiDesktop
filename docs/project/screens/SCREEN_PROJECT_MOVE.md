# SCREEN_PROJECT_MOVE — Project "Move" (ProjectScreen)

> Project 01 "Move" — learn to guide Zowi precisely and understand how a
> two-legged robot moves. Design derived from ZowiAppReborn's
> `01_project_mueve.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ✅ **IMPLEMENTED** (v0.8.0).
- **Screen:** generic `src/views/screens/ProjectScreen.qml`, pushed with `projectId: "move"`.
- **i18n context:** shared UI strings under `"ProjectScreen.qml"` in all locales; project-specific strings in the data folder.
- **Project id:** `move` — Home tile `move_objects`.
- **Data folder:** `projects/move/` (`project.json`, `page/*.html`, `quiz/*.json`, `strings/*.json`, registered in `projects.qrc`, listed in `projects/index.json`).
- **Achievement:** `flapping` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`hex_path` empty) — no install button.
- **URL:** `https://eduardomillan.github.io/ZowiDesktop/docs/projects/move/<locale>/`
  (stored as the relative `move/<locale>/` in `projects/move/strings/<locale>.json`,
  joined with the global `base_url` from `projects/index.json`).
- **Images:** tile `qrc:/images/android/move_button.png`; detail
  `qrc:/images/projects/move_thumb.png` (via `image_key`).
- Reached from Home *Projects* page tile → push; back → pop.

## Contents

- Title ("Move"), learning description and project image.
- Done icon — `move_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz via reusable `QuizComponent`, in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `hex_path`.

## Quiz

Quiz source: `projects/move/quiz/<locale>.json` (inline translated text; English
is the fallback locale).

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

- Core: `zowi::ProjectsStore` loads `projects/index.json` + `projects/<id>/project.json` (resource or filesystem).
- GUI: `ProjectsController` wraps the core store, resolves `strings/<locale>.json` and `quiz/<locale>.json` (fallback `en_US`), and exposes the QML API.
- Quiz: `QuizComponent.qml` (reusable) handles question flow, answer validation,
  blockade timing, and persistence via `SessionController.saveString/getString`.
- Preferences: `ProjectsPreferencesStore` stores `blockade_duration_ms`,
  `achievements_enabled`, `quiz_enabled` in `projects_preferences.json`.
- i18n: Shared UI strings once in the `"ProjectScreen.qml"` context of all
  locales; quiz/strings per locale in the project's data folder; quiz widget
  strings in the `"QuizComponent.qml"` context.