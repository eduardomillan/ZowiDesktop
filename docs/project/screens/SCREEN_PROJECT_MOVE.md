# SCREEN_PROJECT_MOVE — ProjectMoveScreen.qml

> Project 01 "Move" — learn to guide Zowi precisely and understand how a
> two-legged robot moves. Design derived from ZowiAppReborn's
> `01_project_mueve.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectMoveScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectMoveScreen.qml"` (planned).
- **Project id:** `move` — Home tile `move_objects`.
- **Source JSON:** `projects/move.json` (registered in `projects.qrc`).
- **Achievement:** `flapping` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/projects/move-objects/`
- **Images:** tile `qrc:/images/android/move_button.png`; detail
  `qrc:/images/android/project_move_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Move"), learning description and project image.
- Done icon — `move_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | The movements of Zowi allow it to… — "…spin in circles." / **"…walk forward and diagonally."** / "…walk backward and diagonally." |
| 2 | If I increase the walking speed of Zowi, does it always move faster? — **"No. It moves more quickly, but sometimes it does not advance faster because it slips."** / "No. If it moves too fast it can break and stop advancing." / "Yes. Zowi always advances faster if the movement speed is higher." |

All correct → `move_project_completeness = true` + done icon; wrong answer →
`move_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No achievement
dialog until the ACHIEVEMENTS toggle is enabled (see SCREEN_PROJECTS.md).

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("move")`, `isCompleted`,
  `isQuizBlocked`, `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).