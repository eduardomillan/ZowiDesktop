# i18n: migrating from JSON to gettext `.po` files

Status: **proposal, not started**. Goal: use the same translation workflow as
[eduwifi-watchdog](https://github.com/eduardomillan/eduwifi-watchdog)
(`po/LINGUAS`, `po/<prog>.pot`, `po/<lang>.po`, `update.sh` / `compile.sh`).

## Current system

- 5 files `i18n/zowi_<locale>.json` (`en_US`, `es_ES`, `ca_ES`, `fr_FR`,
  `bg_BG`), ~370 lines each, grouped by screen:
  `{ "SettingsScreen.qml": { "restore_desc": "..." } }`.
- Keys are **symbolic ids** (`restore_desc`), not English text. `en_US.json`
  is the fallback when a key is missing in the current locale.
- QML: 32 files define a local `tr("key")` that calls
  `Translator.translate("<Screen>.qml", key)` (~266 call sites). A few more in
  CLI/GUI C++.
- `zowi::TranslationEngine` (core, Qt-free) parses JSON with `nlohmann::json`
  through a replaceable `TranslationLoader` (filesystem by default, `:/i18n/`
  qrc in Qt hosts). Locale list hard-coded in `availableLocales()`; the GUI
  picks it from the session key `locale` or `QLocale::system().name()`.
- `i18n.qrc` lists the JSON files by hand.
- `docs/project/WEB_VERSION.md` and `ANDROID_VERSION.md` assume the JSON files.

## JSON (current) vs `.po`: pros and cons

### Current JSON system
Pros:
- Already works and is tested; zero migration cost.
- Trivial to load anywhere: `nlohmann::json` in core, `fetch()` + `JSON.parse`
  on web, any language on Android. No extra parser to maintain.
- No external tools needed to maintain the files (any editor).
- Symbolic keys are stable: fixing a typo in the English text does not touch
  call sites or other locales.
- Short call sites (`tr("restore_desc")`) and per-screen grouping is explicit.
- Same format as the per-project `strings/` and `quiz/` files.

Cons:
- No tooling for translators: they edit raw JSON (escaping, commas, braces)
  and see keys, not the English original side by side.
- **No change tracking**: if the English text changes, other locales keep the
  old translation silently — nothing marks it as outdated.
- Missing/obsolete keys must be found by hand or with ad-hoc scripts; no
  `--statistics`-style report.
- No check that `%1` placeholders survive translation.
- No extraction from sources: a key used in code but missing in JSON is only
  noticed at run time (falls back to English or shows the key).
- No translator comments, no plural support.
- Not supported by Weblate/Poedit/Lokalize workflows without plugins; different
  from eduwifi-watchdog.

### gettext `.po`
Pros:
- Standard format with mature tools (Poedit, Lokalize, Weblate, `msgfmt`,
  `msgmerge`).
- English text as `msgid`: translators see the original; the code reads
  naturally and works untranslated.
- `msgmerge` marks changed strings *fuzzy* and keeps obsolete ones commented
  out → translations never go stale silently.
- `xgettext` extracts strings from QML/C++, so code and catalogue cannot drift.
- `msgfmt --check --statistics`: syntax, `qt-format` placeholder and coverage
  checks, easy to run in CI.
- Translator comments (`#.`), source references (`#:`), plural forms available.
- Same workflow as eduwifi-watchdog.

Cons:
- One-off migration cost: conversion script, ~266 QML call sites rewritten,
  tests and docs updated (see *Affected files*).
- Need to write and maintain a `.po` parser in core (~100 lines) instead of
  reusing `nlohmann::json`.
- Whoever regenerates the catalogue (`update.sh`) needs the gettext tools
  (`apt install gettext` on Linux). Users and the GUI/CLI builds on any
  platform (Windows included) do not: the `.po` files are plain UTF-8 text
  embedded in the qrc and read by the core parser, just like the JSON today.
- Editing an English string changes the `msgid`: every locale goes *fuzzy*
  until reviewed (intended, but more work for small typo fixes).
- Non-literal calls (`tr(modelData.label)`) need extraction markers.
- Longer call sites (full English text in QML).
- Project JSON (strings/quiz) is not supported by `xgettext` → needs a custom
  extractor script; lesson HTML needs `po4a` (or stays per-locale).

## Proposed design

1. **Layout** (mirrors eduwifi-watchdog):
   `po/LINGUAS`, `po/zowi.pot`, `po/{es,ca,fr,bg}.po`, `po/update.sh`
   (`xgettext` + `msgmerge`/`msginit`), `po/check.sh` (`msgfmt --check
   --statistics`).
2. **`msgctxt` = screen name** (`"SettingsScreen.qml"`), keeping today's
   per-screen contexts.
3. **`msgid` = English text** instead of the symbolic key. This is where
   gettext pays off: translators see the original, and when the English
   changes `msgmerge` marks the entry *fuzzy* instead of silently keeping a
   stale translation. English becomes the source; `en_US.json` disappears and
   the fallback is the `msgid` itself.
   - Requires rewriting call sites `tr("restore_desc")` →
     `tr("Load the BASE firmware…")`. Scriptable: `en_US.json` already maps
     key → English per context.
4. **Extraction**: `xgettext -L JavaScript -ktr` over `src/views/**/*.qml`,
   `-L C++` over CLI/GUI sources; `msgcat` into `zowi.pot`.
   (Verify xgettext's JavaScript parser copes with all QML files; fall back to
   a small custom extractor if not.)
5. **Placeholders**: ~32 strings use Qt-style `%1`. Mark them
   `#, qt-format` so `msgfmt --check` validates that translations keep them.
6. **Runtime: parse `.po` directly in core** with a small parser
   (`msgctxt` / `msgid` / `msgstr`, multi-line strings, escapes; skip
   `#, fuzzy` and empty `msgstr`). No libintl / `.mo`:
   - No new build or runtime dependency on any platform: the Windows (MSVC)
     CI, web and Android builds stay exactly as with JSON. Linking libintl
     would add one (a DLL to ship on Windows), which is why it is avoided.
   - Only developers running `update.sh` need the gettext tools.
   - Keeps core Qt-free and portable to web/Android — only the loader changes.
   - ~100 lines; no plurals needed today (could add `msgid_plural` later).

## Per-project content (`projects/<id>/`)

11 projects (`projects/index.json`), each with one file per locale (5 locales):

| Path | Content | Loaded by |
|---|---|---|
| `strings/<locale>.json` | `title`, `learning_description`, `url`, optional `action_label` (3 projects) | `ProjectsController::getProject` (fallback `en_US`) |
| `quiz/<locale>.json` | `questions[].text`, `answers[].text` + `correct` flag | `ProjectsController` (fallback `en_US`) |
| `page/<locale>.html` | lesson body (HTML paragraphs) | `ProjectsController` (`%1/page/%2.html`) |

Total: 110 JSON + 55 HTML files. Keys in `project.json` (`title_key`,
`description_key`, `url_key`) point into `strings/`.

Findings (checked with a script):
- `url` is **not a translation**: it is always `<id>/<lang>/` (`en`, `es`,
  `ca`, `fr`, `bg`) → derive it from the locale in code and drop it from the
  catalogue.
- The `correct` flags and the answer order are identical in every locale, but
  nothing enforces it; today a translator could break the quiz by reordering
  answers.

### Proposal
- **Structure stays in one locale-neutral file per project**, written in
  English: `projects/<id>/quiz.json` (questions, answers, `correct`) and the
  English `title` / `learning_description` / `action_label` (in `project.json`
  or a single `strings.json`). Per-locale JSON files disappear.
- **Text goes to `.po`**, in a second gettext domain so app UI and lesson
  content can be translated/reviewed separately:
  `po/projects/projects.pot` + `po/projects/<lang>.po`, one file per language
  for all projects (translators handle 1 file, not 11).
  - `msgctxt` = `"<id>"` for strings and `"<id>/quiz"` for quiz text;
    `msgid` = English text.
  - Translator comments (`#.`) for context: "quiz answer (correct)", "project
    title", etc.
- **Extraction**: `xgettext` cannot read these JSON files → small script
  (`po/projects/extract.py`) that walks `projects/index.json` and writes the
  `.pot`; `msgmerge` as usual afterwards.
- **Runtime**: `ProjectsController` asks `TranslationEngine` (same `.po`
  parser, loaded with the `projects` domain) instead of reading per-locale
  JSON. Removes the dynamic-key fallback through `Project<Id>Screen.qml`.
- **Lesson pages `page/<locale>.html`**: two options —
  1. Keep per-locale HTML (they are long prose, translated as a whole). Lowest
     cost; they keep the "silently stale" problem.
  2. `po4a` (xhtml module) to extract paragraphs into the same `projects`
     domain and regenerate `page/<lang>.html` at build/update time.
     Consistent, but generated HTML must be committed or built (Windows CI
     has no po4a) — prefer committing the generated files.
  Recommendation: option 1 first, option 2 as a follow-up.

## Migration steps

1. One-off script: JSON → `.pot` + `.po` (msgctxt = screen, msgid = English
   from `en_US.json`, msgstr = locale value) and rewrite `tr("key")` /
   `translate(ctx, "key")` call sites to the English text.
2. `TranslationEngine`: `.po` parser, fallback to `msgid`, loader reads
   `po/<lang>.po` (filesystem) / `:/po/` (qrc).
3. Replace JSON entries in `i18n.qrc` with the `.po` files (or auto-generate
   it like `views.qrc`).
4. Update `test_translation_engine.cpp`; add a test that every `tr(...)` in
   the sources is in `zowi.pot` and vice versa (guards the mass rewrite).
5. Update `AGENTS.md`, `WEB_VERSION.md`, `ANDROID_VERSION.md`.
6. Projects: script converting `strings/` + `quiz/` per-locale JSON into
   English `quiz.json` / strings + `po/projects/<lang>.po` (asserting that
   `correct` flags and answer order match across locales); adapt
   `ProjectsController` and `project_model`; derive `url`.
7. (Optional, later) `po4a` for `page/<lang>.html`.

## Affected files

### Translation data
| File | Change |
|---|---|
| `i18n/zowi_{en_US,es_ES,ca_ES,fr_FR,bg_BG}.json` | Removed; replaced by `po/zowi.pot` + `po/{es,ca,fr,bg}.po` |
| `po/LINGUAS`, `po/update.sh`, `po/check.sh` | New (modelled on eduwifi-watchdog) |
| `i18n.qrc` | List the `.po` files instead of the JSON (or auto-generate it) |

### Core (Qt-free)
| File | Change |
|---|---|
| `src/core/include/zowi/translation_engine.h` | Doc comments (JSON → `.po`), maybe locale-name handling |
| `src/core/src/translation_engine.cpp` | `.po` parser instead of `nlohmann::json`; fallback to `msgid`; default loader path `po/<lang>.po`; `availableLocales()` |
| `src/core/tests/test_translation_engine.cpp` | Rewrite fixtures/assertions for `.po` and English msgids |

### GUI / CLI adapters
| File | Change |
|---|---|
| `src/gui/controllers/TranslatorController.{h,cpp}` | Qt loader: `:/i18n/zowi_<locale>.json` → `:/po/<lang>.po` |
| `src/gui/controllers/ProjectsController.cpp` | Fallback path translates **dynamic keys** (`titleKey`, `descKey`, `urlKey`) under `Project<Id>Screen.qml`; decide whether to keep or drop it (project strings already come from `projects/<id>/strings/<locale>.json`) |
| `src/gui/main.cpp` | Locale selection (`session "locale"`, `QLocale::system().name()`, `en_US` default) if locale names change |
| `src/gui/CMakeLists.txt`, `src/cli/CMakeLists.txt` | Embed `i18n.qrc` — only if the qrc name/location changes |
| `src/cli/cli_commands.cpp` | `translate` command: source is now English text, not a key; filesystem lookup of `po/` |

### QML call sites (`tr("key")` → `tr("English text")`)
`src/views/main.qml` and:

- `components/`: `ForgetController`, `GestureSelectorContent`,
  `GestureSelectorDialog`, `MouthSelectorContent`, `MouthSelectorDialog`,
  `MovementSelectorContent`, `MovementSelectorDialog`, `QuizComponent`,
  `RankingDialog`, `StatusBar`, `TimelineChip`.
- `screens/`: `CalibrationScreen`, `GameMemoryScreen`, `GameMouthsScreen`,
  `GameTimelineScreen`, `GestureScreen`, `HomeScreen`, `MouthEditorScreen`,
  `MouthScreen`, `MovementSelector`, `PadScreen`, `ProjectScreen`,
  `ScanScreen`, `ScreenTemplate`, `SettingsScreen`, `SoundsScreen`,
  `SplashScreen`, `WelcomeScreen`, `WizardFoundScreen`, `WizardRenameScreen`,
  `WizardScreen`.

**Non-literal calls** that `xgettext` cannot extract and the rewrite script
cannot handle blindly — their models must hold the English text and be marked
for extraction (e.g. a no-op `QT_TR_NOOP`-style marker or `#.` comment):
- `screens/SoundsScreen.qml:138` — `tr(modelData.label)`
- `screens/SettingsScreen.qml:321` — `tr(modelData.label)`
- `screens/SettingsScreen.qml:380` — `tr(modelData.key)`
- `screens/SettingsScreen.qml:386` — `tr(modelData.desc)`

### Tests and scripts
| File | Change |
|---|---|
| `src/cli/tests/blackbox/test_translate.sh` | Inputs/expected outputs use English msgids |
| `scripts/test/run-cli-blackbox.sh`, `src/cli/tests/README.md` | Comments mention `i18n/` relative to repo root → `po/` |

### Documentation
- `AGENTS.md`, `.github/copilot-instructions.md` — i18n paragraph.
- `docs/project/ARCHITECTURE.md` — "JSON-file i18n engine", tree listing.
- `docs/project/WEB_VERSION.md`, `docs/project/ANDROID_VERSION.md` — loader
  examples fetch `i18n/zowi_<locale>.json`.
- `docs/project/ZOWI_CLI_HOWTO.md` — `translate` command ("JSON-based").
- `docs/project/win/WIN_HOWTO.md` — "same `i18n/*.json`".
- `docs/project/screens/SCREEN_*.md` — only mention the i18n context name,
  which is kept as `msgctxt`; no change expected unless they quote keys.
- `CHANGELOG.md` — user-facing entry only if behaviour visibly changes.

### Per-project content
| File | Change |
|---|---|
| `projects/<id>/strings/<locale>.json` (11 × 5) | Removed; English text moves to `project.json` / `strings.json`, translations to `po/projects/<lang>.po`; `url` derived in code |
| `projects/<id>/quiz/<locale>.json` (11 × 5) | Replaced by one English `projects/<id>/quiz.json`; text translated via `po/projects/` |
| `projects/<id>/project.json` (11) | `title_key` / `description_key` / `url_key` replaced by English text (or removed) |
| `projects/<id>/page/<locale>.html` (11 × 5) | Unchanged in phase 1; generated by `po4a` in phase 2 |
| `po/projects/projects.pot`, `po/projects/<lang>.po`, `po/projects/extract.py` | New |
| `src/core/include/zowi/project_model.h` + its source/tests | Fields `titleKey` / `descriptionKey` / `urlKey` and comments about `strings/` / `quiz/<locale>.json` |
| `src/gui/controllers/ProjectsController.cpp` | `getProject` / quiz loading: read English structure + translate through the `projects` domain; build `url` from `<id>/<lang>/`; drop the `Project<Id>Screen.qml` key fallback |
| `src/views/screens/ProjectScreen.qml`, `components/QuizComponent.qml` | Only comments/field names if the model changes |
| `cmake/GenerateQrc.cmake` (`projects.qrc`) | Nothing if globbing still works; check that `po/projects/*.po` gets embedded (new qrc entry) |
| `docs/project/PROJECTS_HOWTO.md`, `docs/project/screens/SCREEN_PROJECTS.md`, `docs/project/ZOWI_CLI_HOWTO.md` | Describe the new layout and how to add a project/translation |

### Out of scope
- `src/config.json` — no i18n keys besides the session `locale`.

## Open decisions

- **Locale names**: keep `es_ES`, `ca_ES`… or switch to `es`, `ca@valencia`
  like eduwifi-watchdog. Affects the session key `locale`, `availableLocales()`
  and the language selector.
- **Keys vs English msgids**: a cheaper variant keeps the symbolic keys as
  `msgid` (no call-site rewrite) but loses the main gettext benefits
  (translators see ids, no fuzzy tracking). Not recommended.

## Risks

- Mass rewrite of ~266 QML call sites → mitigated by the round-trip test.
- Identical English text in the same screen with different meanings would
  collide under one `msgid` — check for duplicates during conversion and add a
  disambiguating `msgctxt` suffix if any appear.

## Benefits

Standard tooling (Poedit, Lokalize, Weblate), fuzzy/untranslated reporting,
same workflow as eduwifi-watchdog, cleaner diffs for translators.
