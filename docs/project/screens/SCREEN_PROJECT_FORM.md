# SCREEN_PROJECT_FORM — ProjectFormScreen.qml

> Project 03 "Form" — the shape of Zowi and how its parts fit, plus the
> customisation stickers. Design derived from ZowiAppReborn's
> `03_project_forma.json`; shared screen design in
> [SCREEN_PROJECTS.md](SCREEN_PROJECTS.md).

- **Status:** ⚠️ **NOT IMPLEMENTED** — design proposal pending review.
- **File:** `src/views/screens/ProjectFormScreen.qml` (does not exist yet).
- **i18n context:** `"ProjectFormScreen.qml"` (planned).
- **Project id:** `form` — Home tile `robot_form`.
- **Source JSON:** `projects/form.json` (registered in `projects.qrc`).
- **Achievement:** `confused` (reserved for the deferred ACHIEVEMENTS layer).
- **Firmware:** none (`project_hex` empty) — no install button.
- **URL:** `http://zowi.bq.com/proyectos/form`
- **Images:** tile `qrc:/images/android/robot_form_button.png`; detail
  `qrc:/images/android/project_form_image.png`.
- Reached from Home *Projects* page tile → push; back → pop.

## Contents (planned)

- Title ("Form"), learning description ("La forma de Zowi es bonita ¿verdad?…
  ¡Además es personalizable! Juega a cambiar su apariencia con las pegatinas
  de la caja.") and project image.
- Done icon — `form_project_completeness` (done / not-done).
- Project link — opens the URL in the system browser.
- **Run Test** — quiz (below), in-screen; disabled during a quiz blockade.
- **No firmware install** — this project has no `project_hex`.

## Quiz (planned)

| Q | Answers (correct in bold) |
|---|---|
| 1 | La cabeza de Zowi tiene ese tamaño y forma… — "…para que se mantenga en equilibrio." / **"…para que encajen los circuitos y motores."** / "…para que le queden bien las pegatinas." |
| 2 | Las pegatinas de Zowi: — "Cambian el funcionamiento del robot." / "Son permanentes." / **"Solo sirven para cambiar su aspecto."** |

All correct → `form_project_completeness = true` + done icon; wrong answer →
`form_project_quiz_blockade` (10 min lock, `mm:ss` countdown). No achievement
dialog until the ACHIEVEMENTS toggle is enabled.

## Signals / QML context (planned)

- `backClicked()` → pop; in-screen quiz signals `quizFinished` / `quizBlocked`.
- `Robot` (connected/appId/battery), `Session.loadActiveZowiName()`,
  `Projects` context (`getProject("form")`, `isCompleted`, `isQuizBlocked`,
  `blockQuiz`, `setCompleted`), `Translator`.
- No commands sent (quiz is local; no firmware for this project).