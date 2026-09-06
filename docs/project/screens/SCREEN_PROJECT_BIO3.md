# SCREEN_PROJECT_BIO3 — ProjectBio3Screen.qml

> Project 05 "Biology III" — how Zowi moves: servomotors and why legs vs wheels.
> Design derived from ZowiAppReborn's `05_project_bio3.json`; shared screen
> design in [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectBio3Screen.qml` (does not exist yet).
- **i18n context:** `"ProjectBio3Screen.qml"` (planned).
- **Project id:** `bio3` — Home tile `robot_feet`.
- **Source JSON:** `projects/bio3.json` (registered in `projects.qrc`).
- **Achievement:** `jitter` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/projects/bio3/`
- **Images:** tile `qrc:/images/android/feet_button.png`; detail
  `qrc:/images/android/project_bio3_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Biology III"), learning description ("Los robots pueden usar
  distintos tipos de componentes para moverse. ¿Sabes cuál lleva Zowi?")
  and project image.
- Done icon — `bio3_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | Para ir por un terreno llano, la forma más rápida de moverse es con: — **"Ruedas."** / "Orugas." / "Patas." |
| 2 | ¿Qué tienen las patas de Zowi? — "Servos de rotación continua." / **"Servos."** / "Motores de coche." |

All correct → `bio3_project_completeness = true` + done icon; wrong answer →
`bio3_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No achievement
dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("bio3")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).