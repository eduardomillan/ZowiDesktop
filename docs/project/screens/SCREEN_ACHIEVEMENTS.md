# SCREEN_ACHIEVEMENTS — AchievementsScreen.qml

> The collection of achievements, grouped like the Android app: **Movements**,
> **Animations** and **Games**. Each row shows the badge (a padlock while locked),
> the title, the description and, while locked, how to unlock it. The rules, the
> configuration and the storage are in [../ACHIEVEMENTS_HOWTO.md](../ACHIEVEMENTS_HOWTO.md).

- **File:** `src/views/screens/AchievementsScreen.qml`
- **i18n context:** `"AchievementsScreen.qml"` (also read by `AchievementTexts.qml`,
  which the unlock dialog and the locked-content hint share)
- Pushed by `HomeScreen.achievementsClicked` (the Achievements button, hidden when
  `achievements_enabled` is `false`); back → pop.
- Template: `ScreenTemplate` (status bar, title `title`, subtitle `subtitle` = "Unlocked: x / 17").

## Signals

| Signal | Emitted by | Consumed in |
|--------|-----------|-------------|
| `backClicked()` | inherited from `ScreenTemplate` | `main.qml`: `stack.pop()` |

## QML context used

- `Achievements`: `list()` (`[{ id, type, unlocked }]`), `unlockedCount`, `total`,
  `typeOf(id)`, `daysOfUse`, `timelineChips`, `memoryScore`, `mouthsLevel` (for the
  "how to unlock" texts), `easterEggEnabled`, `easterEggStep(n)`; signal
  `achievementsChanged` (the list is read again).
- `Projects.projectTitleForAchievement(id)`: the lesson that awards a project achievement.
- `Translator`, `Config` (colours).

## Layout

- A centred scrolling column (max 760 px) with three rounded "maker box" cards, one per
  group. Rows are 84 px: a 64 px badge on the left and, to its right, the title (bold,
  upper case), the description and the unlock condition (italic, grey, only while locked).
  Locked rows are grey; unlocked ones use the primary colour and the achievement's own
  badge.
- **Easter egg:** three transparent 18 px areas on the window corners (bottom-left,
  top-right, top-left), above everything, parented to the screen so they sit on the real
  corners. Pressing them in that order calls `Achievements.easterEggStep(1..3)` and the
  third one unlocks everything. Disabled with `achievements_easter_egg: "false"`.

## i18n

Keys under `"AchievementsScreen.qml"` in the 5 locales: `title`, `subtitle` (`%1` of `%2`),
`section_movement`, `section_animation`, `section_game`, `title_<id>` and `desc_<id>`
for each of the 17 achievements, and the unlock conditions `cond_start`,
`cond_second_launch`, `cond_days` (`%1`), `cond_both`, `cond_anxious` (`%1`),
`cond_in_love` (`%1`), `cond_mouths` (`%1`) and `cond_project` (`%1` = lesson title).
The titles and descriptions of es / fr are the Android app's; ca / bg are translations
of them. The unlock dialog uses `main.qml` keys (`achievement_unlocked_title`,
`achievement_unlocked_text`, `achievement_continue`, `achievement_locked_hint`) and
Settings the `delete_achievements_*` ones.
