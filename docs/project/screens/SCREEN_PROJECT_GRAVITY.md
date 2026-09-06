# SCREEN_PROJECT_GRAVITY — ProjectGravityScreen.qml

> Project 10 "Gravity" — balance, the centre of gravity and calibration.
> Design derived from ZowiAppReborn's `10_project_gravity.json`; shared screen
> design in [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectGravityScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectGravityScreen.qml"` (planned).
- **Project id:** `gravity` — Home tile `gravity`.
- **Source JSON:** `projects/gravity.json` (registered in `projects.qrc`).
- **Achievement:** `sleepy` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/gravity`
- **Images:** tile `qrc:/images/android/gravity_button.png`; detail
  `qrc:/images/android/project_gravity_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Gravity"), learning description ("Descubre cómo mantiene Zowi el
  equilibrio aprendiendo lo que es el \"centro de gravedad\".") and project image.
- Done icon — `gravity_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | Los pies de Zowi son así de grandes… — **"…para mantener mejor el equilibrio."** / "…para andar más rápido." / "…para eliminar la fuerza de la gravedad." |
| 2 | El centro de gravedad de Zowi: — "No existe." / "Está en los motores de sus patas." / **"Está más o menos entre sus ojos."** |

All correct → `gravity_project_completeness = true` + done icon; wrong answer
→ `gravity_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No
achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("gravity")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).