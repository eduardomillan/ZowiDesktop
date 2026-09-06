# SCREEN_PROJECT_BIO1 — ProjectBio1Screen.qml

> Project 04 "Biology I" — Zowi's "eyes": how the ultrasonic sensor works and
> whether it sees in the dark. Design derived from ZowiAppReborn's
> `04_project_bio1.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectBio1Screen.qml` (does not exist yet).
- **i18n context:** `"ProjectBio1Screen.qml"` (planned).
- **Project id:** `bio1` — Home tile `robot_eyes`.
- **Source JSON:** `projects/bio1.json` (registered in `projects.qrc`).
- **Achievement:** `wave` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/bio1`
- **Images:** tile `qrc:/images/android/eyes_button.png`; detail
  `qrc:/images/android/project_eyes_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Biology I"), learning description ("…Apaga las luces y comprueba si
  se choca con los objetos o los evita como cuando hay luz.") and project image.
- Done icon — `bio1_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | La verdad es que "los ojos" de Zowi… — "…son como los de los humanos. Si no hay luz no ven nada." / **"…son como los de un gato, ven aunque sea de noche."** / **"…no ven la luz. En realidad emiten ultrasonido."** ⚠️ the Android JSON marks **both** answers 2 and 3 as correct (`is_correct: true`) — data quirk to confirm when porting |
| 2 | Los "ojos" de Zowi: — **"Envían sonidos que rebotan en los objetos y vuelven."** / "Son como los ojos de los superhéroes, funcionan con rayos X." / "Funcionan con rayos ultravioletas." |

All correct → `bio1_project_completeness = true` + done icon; wrong answer →
`bio1_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No achievement
dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("bio1")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).