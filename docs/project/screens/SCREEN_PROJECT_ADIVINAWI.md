# SCREEN_PROJECT_ADIVINAWI — ProjectAdivinawiScreen.qml

> Project 09 "Adivinawi" (fortune-telling robot) — reprogram Zowi to answer
> at random. The only project (with Reprogram) that installs firmware,
> launched **from this project's own window**. Design derived from
> ZowiAppReborn's `09_project_adivinawi.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectAdivinawiScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectAdivinawiScreen.qml"` (planned).
- **Project id:** `adivinawi` — Home tile `adivinawi`.
- **Source JSON:** `projects/adivinawi.json` (registered in `projects.qrc`).
- **Achievement:** `magic` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** `ZOWI_Adivinawi_v2.hex` (`project_hex`) → flash from this screen.
- **URL:** `http://zowi.bq.com/proyectos/adivinawi`
- **Images:** tile `qrc:/images/android/adivinawi_button.png`; detail
  `qrc:/images/android/project_adivinawi_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Adivinawi"), learning description ("¿Crees que es posible adivinar
  las cosas antes de que ocurran?… descubre lo que significa aleatorio").
  and project image.
- Done icon — `adivinawi_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Install firmware** — only here, because `project_hex != ""`. Conn-gated +
  50 % battery check → `Robot.restoreFirmware("qrc:/firmware/ZOWI_Adivinawi_v2.hex")`
  with the existing progress dialog / low-battery confirmation / success or
  error (still-connected vs not) dialogs; resolved from this window, not Home.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- Firmware flows disable the quiz/dependant controls while flashing.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | ¿Cómo decide Zowi qué responder? — "Sabe las respuestas porque es un robot adivino." / **"Está programado para decidir al azar."** / "Está programado para cambiar de respuesta cada tres golpecitos." |
| 2 | ¿Es posible que Zowi responda siempre Sí en el modo 1? — **"Es muy muy difícil, pero podría pasar."** / "Si queda poca batería, ocurrirá siempre." / "No, es completamente imposible." |

All correct → `adivinawi_project_completeness = true` + done icon; wrong answer
→ `adivinawi_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No
achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; `installFirmwareClicked(hex)` → `Robot.restoreFirmware`.
- `Robot`: `connected`, `appId`, `battery >= 50`, firmware signals
  `onFirmwareRestoreStarted/Progress/Finished/BatteryLow`.
- `Session.loadActiveZowiName()` (firmware dialogs reference the name);
  `Projects` context (`getProject("adivinawi")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`); `Translator`.
- Firmware is the only command sent; the quiz is local.