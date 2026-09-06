# SCREEN_PROJECT_REPROGRAM — ProjectReprogramScreen.qml

> Project 06 "Reprogram" (Robot Alarma) — teach Zowi new skills by flashing the
> **Alarm/Guardian firmware**. The only project (with Adivinawi) that installs
> firmware, launched **from this project's own window**. Design derived from
> ZowiAppReborn's `06_project_reprogram.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectReprogramScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectReprogramScreen.qml"` (planned).
- **Project id:** `reprogram` — Home tile `robot_alarm`.
- **Source JSON:** `projects/reprogram.json` (registered in `projects.qrc`).
- **Achievement:** `angry` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** `ZOWI_Alarm_v2.hex` (`project_hex`) → flash from this screen.
- **URL:** `http://zowi.bq.com/proyectos/reprogram`
- **Images:** tile `qrc:/images/android/alarm_button.png`; detail
  `qrc:/images/android/project_alarm_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Reprogram"), learning description ("…vamos a convertir a Zowi en una
  alarma antirrobo y ¡en un robot guardián!") and project image.
- Done icon — `reprogram_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Install firmware** — only here, because `project_hex != ""`. Conn-gated +
  50 % battery check → `Robot.restoreFirmware("qrc:/firmware/ZOWI_Alarm_v2.hex")`
  with the existing progress dialog / low-battery confirmation / success or
  error (still-connected vs not) dialogs; resolved from this window, not Home.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- Firmware flows disable the quiz/dependant controls while flashing.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | Al reprogramar a Zowi con el programa de la Alarma: — "El botón de encendido no funciona." / "Todo sigue igual." / **"Los botones A y B hacen otra cosa."** |
| 2 | El nuevo programa de Alarma… — "…convierte a Zowi en un despertador." / "…convierte a Zowi en un detector de humos." / **"…convierte a Zowi en un guardián que avisa de movimientos a su alrededor."** |

All correct → `reprogram_project_completeness = true` + done icon; wrong answer
→ `reprogram_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No
achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; `installFirmwareClicked(hex)` → `Robot.restoreFirmware`.
- `Robot`: `connected`, `appId`, `battery >= 50`, firmware signals
  `onFirmwareRestoreStarted/Progress/Finished/BatteryLow`.
- `Session.loadActiveZowiName()` (firmware dialogs reference the name);
  `Projects` context (`getProject("reprogram")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`); `Translator`.
- Firmware is the only command sent; the quiz is local.