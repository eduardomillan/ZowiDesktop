# SCREEN_PROJECT_CHOREOGRAPHY — ProjectChoreographyScreen.qml

> Project 02 "Choreography" — invent a dance routine for Zowi with
> "3, 2, 1 ¡Acción!". Design derived from ZowiAppReborn's
> `02_project_choreography.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectChoreographyScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectChoreographyScreen.qml"` (planned).
- **Project id:** `choreography` — Home tile `choreography`.
- **Source JSON:** `projects/choreography.json` (registered in `projects.qrc`).
- **Achievement:** `swing` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/choreography`
- **Images:** tile `qrc:/images/android/choreography_button.png`; detail
  `qrc:/images/android/project_choreography_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Choreography"), learning description ("Una coreografía es una serie
  de pasos de baile…") and project image.
- Done icon — `choreography_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | En "3, 2, 1 ¡Acción!" los movimientos tienen varias opciones: — "4 velocidades y dirección." / "5 velocidades y el número de repeticiones." / **"3 velocidades, el número de repeticiones y alguno de ellos, dirección."** |
| 2 | Las coreografías que haces con "3, 2, 1 ¡Acción!": — "Sólo pueden llegar a un máximo de 10 movimientos." / **"Pueden combinar movimientos y gestos de boca."** / "Cambian si pones la mano frente a Zowi." |

All correct → `choreography_project_completeness = true` + done icon; wrong
answer → `choreography_project_quiz_blockade` (10 min lock, `mm:ss`
countdown). No achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("choreography")`, `isCompleted`,
  `isQuizBlocked`, `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).