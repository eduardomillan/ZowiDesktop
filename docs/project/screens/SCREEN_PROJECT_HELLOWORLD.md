# SCREEN_PROJECT_HELLOWORLD — ProjectHelloWorldScreen.qml

> Project 07 "Hello World" — first steps programming Zowi with Bitbloq.
> Design derived from ZowiAppReborn's `07_project_helloworld.json`; shared
> screen design in [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectHelloWorldScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectHelloWorldScreen.qml"` (planned).
- **Project id:** `helloworld` — Home tile `hello_world`.
- **Source JSON:** `projects/helloworld.json` (registered in `projects.qrc`).
- **Achievement:** `super_happy` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/helloworld`
- **Images:** tile `qrc:/images/android/bitbloq_button.png`; detail
  `qrc:/images/android/project_helloworld_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Hello World"), learning description ("…Da tus primeros pasos como
  experto en robots programando a Zowi con Bitbloq") and project image.
- Done icon — `helloworld_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | Después de programar a Zowi con Bitbloq y apagar y encender a Zowi… — **"…Zowi realiza lo que has programado."** / "…Zowi recuerda las funciones de inicio (modo 1, modo 2 y modo 3)." / "…Zowi se olvida del programa hasta que cargas uno nuevo." |
| 2 | Los programas de Zowi se escriben en: — "Código Morse." / "Código Morsa." / **"Código Arduino."** |

All correct → `helloworld_project_completeness = true` + done icon; wrong
answer → `helloworld_project_quiz_blockade` (10 min lock, `mm:ss` countdown).
No achievement dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("helloworld")`, `isCompleted`,
  `isQuizBlocked`, `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).