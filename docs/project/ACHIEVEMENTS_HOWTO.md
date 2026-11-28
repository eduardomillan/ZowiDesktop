# Achievements — How To

> How the achievements layer works: the 17 achievements, how each one is earned,
> what it locks, how to switch it on or off, where the progress is stored and how
> to inspect or reset it. Ported from the Android app (ZowiAppReborn).

## Table of Contents

- [Status](#status)
- [Model](#model)
- [How each achievement is earned](#how-each-achievement-is-earned)
- [What achievements lock](#what-achievements-lock)
- [GUI flow](#gui-flow)
- [Configuration (turning the layer on or off)](#configuration-turning-the-layer-on-or-off)
- [Storage, reset and the command line](#storage-reset-and-the-command-line)
- [Tests](#tests)

---

## Status

| Part | Status |
|------|--------|
| Core: catalog, store, unlock rules (`zowi::AchievementsStore`) | ✅ Implemented |
| GUI: `Achievements` controller, collection screen, unlock notice, locked content, Settings reset | ✅ Implemented |
| Command line: `zowi_cli achievements list / unlock / reset` | ✅ Implemented |
| Switches in `config.json` (whole layer, locking, notice, robot celebration, days, easter egg) | ✅ Implemented |

## Model

- **17 achievements** in three kinds, in this order: *movements* (`crusaito`,
  `flapping`, `shake_leg`, `tip_toe`, `jitter`, `ascending_turn`, `swing`),
  *animations* (`super_happy`, `sleepy`, `fart`, `confused`, `in_love`, `angry`,
  `anxious`, `magic`, `wave`) and *games* (`mouths_editor`).
- Only `ascending_turn` is unlocked from the start (as in the Android app).
- Unlocking is permanent until the user deletes the achievements. Unlocking one
  that is already open does nothing, and an unknown id is ignored.
- Each one has a badge (`images/android/<id>_badge.png`, a padlock while locked), a
  title, a description and a "how to unlock" text, all translated
  (`"AchievementsScreen.qml"` context).

## How each achievement is earned

The rules are pure functions in the core (`achievements.h`) and are unit-tested.

| Achievement | Rule |
|-------------|------|
| `crusaito` | Open the app a **second time** (from the second launch on). |
| `fart` | Open the app on **4 different days** (`achievements_days_of_use`). |
| `shake_leg` | Visit the **Gamepad** and the **Timeline** at least once: the first visit of the second one unlocks it. |
| `anxious` | Play a **Timeline** sequence of **15 or more** chips all the way to the end. |
| `in_love` | Reach **12 points** in **Memory** (Zowi says: sequence length − 1). |
| `mouths_editor` | Reach **level 8** in **Draw the mouths** (score ≥ 7). It also opens the Mouths Editor tile. |
| `flapping`, `swing`, `jitter`, `confused`, `sleepy`, `magic`, `wave`, `angry`, `super_happy`, `tip_toe` | **Pass the quiz** of the project that awards it (`achievement_id` in `projects/<id>/project.json`). |
| `ascending_turn` | Unlocked from the start. |

The game thresholds are the ones of the games' own configuration
(`ZowiDiceConfig::achievementThreshold`, `MouthsGameConfig::achievementLevelThreshold`),
so the rule and the game never disagree. A project whose `achievement_id` is not one
of the 17 (for example the "mouth" project) simply awards nothing.

An easter egg from the original unlocks everything: on the achievements screen press
the three hidden corner areas in order — **bottom-left, top-right, top-left**.

## What achievements lock

Only while the layer **locks content** (`achievements_lock_content`):

- The five gamepad movement buttons — **Shake leg, Jitter, Swing, Flapping and
  Crusaito** — are greyed out and do nothing until their achievement is earned.
  Pressing one shows how to unlock it.
- The **Mouths Editor** tile on the home screen stays greyed out until `mouths_editor`.

With locking off (or the layer off) everything is available from the start, as before
the layer existed.

## GUI flow

- **Home screen:** the Achievements button (next to Ranking) opens the collection;
  hidden when the layer is off. The app start is logged once per launch, once the Home
  screen is up (`Achievements.appStarted()`).
- **Collection screen** (`AchievementsScreen.qml`, see `screens/SCREEN_ACHIEVEMENTS.md`):
  the three groups with every badge, title, description and, while locked, how to get it.
- **Unlock notice:** a dialog "You have unlocked: …" with the badge (`AchievementUnlockDialog.qml`,
  owned by `main.qml`); several unlocks at once are shown one after another. When the
  robot is connected it also plays the VICTORY gesture (`H 12`).
- **Settings → Delete achievements:** asks for confirmation and goes back to the initial
  state. Rankings and everything else are not affected.

## Configuration (turning the layer on or off)

All keys are in `config.json` and can be overridden per installation (system file,
environment, user file; see `CONFIG_HOWTO.md`). To switch the whole layer off for an
installation:

```json
{ "achievements_enabled": "false" }
```

in `/etc/ZowiDesktop/config.json` (Windows: `%PROGRAMDATA%\ZowiDesktop\config.json`).

| Key | Default | Meaning |
|-----|---------|---------|
| `achievements_enabled` | `true` | Master switch. `false`: nothing is recorded, shown or locked; the Home button and the Settings entry are hidden, no notice, no gating. Saved progress is kept and comes back when it is turned on again. |
| `achievements_lock_content` | `true` | `false`: achievements are only collectibles and never lock the gamepad movements or the Mouths Editor. |
| `achievements_notify` | `true` | `false`: no "You have unlocked" dialog (the achievement is still recorded). |
| `achievements_robot_celebration` | `true` | `false`: the robot does not play VICTORY on an unlock. |
| `achievements_days_of_use` | `4` | Distinct days of use needed for `fart`. |
| `achievements_easter_egg` | `true` | `false`: the hidden corner buttons do nothing. |

## Storage, reset and the command line

- The progress lives in its own file, **`ZowiAchievements.json`**, next to `ZowiApp.json`
  (e.g. `~/.config/ZowiDesktop/`): the unlocked ids, the app-start counter, the days of
  use and the first visits to the gamepad and the Timeline. Because it is a different
  file, `zowi_cli session clear` never touches it, and a corrupt file reads as the
  initial state.
- `zowi_cli achievements list` shows every achievement and whether it is unlocked;
  `zowi_cli achievements unlock <id|all>` unlocks one or all; `zowi_cli achievements
  reset` goes back to the initial state. See `ZOWI_CLI_HOWTO.md`.

## Tests

- Core: `test_achievements` (`ctest --test-dir build -R achievements`): catalog, initial
  state, unlock / unlock all / reset, usage counters and every rule.
- Manual: play with a fresh config folder (`XDG_CONFIG_HOME=/tmp/x ./build/ZowiDesktop`):
  open the app twice, pass a project quiz, reach level 8 in Draw the mouths, and check
  the collection screen, the locked gamepad buttons and Settings → Delete achievements.
