# Ranking — How To

> How the local game ranking works today, who can delete it, and what is still
> pending. Single source of truth; per-game notes live in the
> `screens/SCREEN_GAME_*.md` files. Planning history: `.local/ranking_planning.md`.

---

## Status

| Part | Status |
|------|--------|
| Local top-10 store (core) | ✅ Implemented (`RankingStore`) |
| Timeline score formula (core) | ✅ Implemented (`timelineScore`), not wired to the UI yet |
| Memory (Zowi Says) ranking in the GUI | ✅ Implemented |
| Mouths (Pintabocas) ranking in the GUI | ✅ Implemented |
| Timeline ranking in the GUI | 🚧 Pending (the Ranking button is still a disabled placeholder) |
| Online ranking | 🚧 Pending (design only, see the planning doc) |
| Player profiles | ❌ Not planned yet (see [Known limits](#known-limits)) |

---

## Data model

- `zowi::RankingStore` (`src/core/include/zowi/ranking_store.h`) — Qt-free.
- `RankingEntry { int points; std::string playerName; int64_t timestamp; }`
  (timestamp in seconds since epoch). Same shape as the Android original
  (`points`, `playerName`, `timestamp`).
- Games: `RankingGame::{ZowiSays, Mouths, Timeline}`, addressed from QML/CLI as
  `"zowi_says"`, `"mouths"`, `"timeline"`.
- **Own file:** rankings are stored in `ZowiRanking.json`, in the same config
  directory as the session file `ZowiApp.json` (e.g. `~/.config/ZowiDesktop/`
  on Linux). Each game's list is a JSON array stored as a string under the key
  `zowi_says` / `mouths` / `timeline`. Because it is a different file,
  clearing or resetting the session never touches the ranking.
- Corrupt or missing data is treated as an empty ranking.

## Rules

| Rule | Value |
|------|-------|
| Entries kept per game | top **10** |
| Minimum score to enter | Zowi Says **3**, Mouths **2**, Timeline **1** (`RankingStore::minScoreToQualify`) |
| Full list (10 entries) | a new score must be **strictly higher** than the last one |
| Ties | the new entry goes **after** existing entries with the same score |
| Nickname | trimmed, truncated to **12** characters, `?` if blank |

Scores: Zowi Says = sequence length − 1, Mouths = level − 1 (as in Android).
The minimums mirror Android (`size > 3`, `level > 2`). The unused config fields
`ZowiDiceConfig::rankThreshold` and `MouthsGameConfig::rankScoreThreshold` are
**not** read by the ranking; the minimums live in `RankingStore`.

### Timeline score (not wired yet)

`timelineScore()` (`src/core/include/zowi/timeline_score.h`) awards points for a
**completed** sequence: movement 3 × repetitions, gesture 2 × repetitions, mouth 1
(ignores repetitions); movement speed Fast +20 % / Slow −20 %; +2 per distinct
item type used; only the first 40 steps count; at least 5 steps to qualify.
All numbers live in `TimelineScoreConfig`.

## GUI flow

- `RankingController` (`Ranking` in QML) exposes `top(game)`, `qualifies(game,
  score)`, `submit(game, name, score)` and `best(game)`. It has **no delete
  method** by design.
- `RankingDialog.qml` (shared component, i18n context `RankingDialog.qml`):
  - `showList(highlight)` — the top-10 list (position badge, name, points); the
    row at `highlight` is shown in the hover color; empty state text.
  - `showScore(score)` — if the score qualifies, asks for a nickname (the last
    used one is remembered in the session key `ranking_last_name`), saves it,
    and shows the list with the new entry highlighted. If it does not
    qualify, it just shows the list.
- Memory and Mouths: the corner **Ranking** button opens the list. On game
  over, if the score qualifies, the dialog offers **"Save to ranking"**; Close
  and Retry discard an unsaved score. In Memory, "New best!" is shown when the
  score beats the stored best (previously a fixed score ≥ 12).

## Deleting rankings (admin only)

- The app has **no way** to delete rankings: not the achievements reset in
  Settings (its text says rankings are not affected), not any other button.
- Local admin: the CLI.

  ```bash
  zowi_cli ranking list [zowi_says|mouths|timeline|all]
  zowi_cli ranking clear [zowi_says|mouths|timeline|all]
  ```

  Removing the file `ZowiRanking.json` by hand also works. `zowi_cli session
  clear` does not affect rankings. See `ZOWI_CLI_HOWTO.md`.
- Online (future): only the maintainer, by editing the published JSON on
  `gh-pages`; no delete endpoint will exist in the client or the proxy.

## Known limits

- **No player profiles.** Several people using the same OS user and app share one
  ranking per game; entries are told apart only by the nickname typed at the
  moment of saving ("Ana" and "ana " are different entries). There is no best
  score per player and no per-player achievements. Same as the Android original.
- Scores are produced by the client, so a local ranking can be edited by anyone
  with access to the file. Not a security feature.
- Old ranking keys written to `ZowiApp.json` by early builds (`*_ranking`) are
  ignored; there is no migration.

## Pending (see `.local/ranking_planning.md`)

1. Timeline: emit a `sequenceCompleted(score)` from `TimelineController` and use
   `RankingDialog` from `GameTimelineScreen.qml`.
2. Online ranking: read from a static JSON on GitHub Pages; submissions go
   through a small serverless proxy (GitHub Pages alone is read-only and a token
   cannot ship in the client). Opt-in, nickname only, no accounts.
3. Decide whether to add player profiles (reserve a player id in `RankingEntry`).

## Tests

- Core: `test_ranking_store`, `test_timeline_score`
  (`ctest --test-dir build -R 'ranking|timeline_score'`).
- Manual: play Memory/Mouths until the score qualifies, save a nickname, reopen
  the list from the corner button, then check `zowi_cli ranking list`.
