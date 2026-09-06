# SCREEN_PROJECT_BITBLOQ2 — ProjectBitbloq2Screen.qml

> Project 08 "Bitbloq" (Bitbloq II — Sensors) — program Zowi to react to its
> sensors with conditional blocks. Design derived from ZowiAppReborn's
> `08_project_bitbloq2.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectBitbloq2Screen.qml` (does not exist yet).
- **i18n context:** `"ProjectBitbloq2Screen.qml"` (planned).
- **Project id:** `bitbloq2` — Home tile `bitbloq_sensors`.
- **Source JSON:** `projects/bitbloq2.json` (registered in `projects.qrc`).
- **Achievement:** `tip_toe` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/bitbloq2`
- **Images:** tile `qrc:/images/android/bitbloq2_button.png`; detail
  `qrc:/images/android/project_bitbloq2_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Bitbloq"), learning description ("¡Haz tus primeros programas
  inteligentes! Aprende a programar a Zowi…") and project image.
- Done icon — `bitbloq2_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | Al programar un bloque condicional Si…Ejecutar… — "Hace que siempre se ejecute la instrucción que tenga dentro." / **"Hace que solo se ejecute una instrucción cuando se cumpla una condición."** / "Hace que nunca se ejecute la instrucción que tenga dentro." |
| 2 | El sensor de distancia de Zowi… — "…es un componente digital que mide distancia." / "…es un componente analógico que mide el color de las nubes." / **"…es un componente analógico que mide distancia."** |

All correct → `bitbloq2_project_completeness = true` + done icon; wrong answer
→ `bitbloq2_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No
achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("bitbloq2")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).