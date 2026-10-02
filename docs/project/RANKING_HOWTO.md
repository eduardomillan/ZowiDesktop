# Ranking — How To

> How the game ranking works today, who can delete it, and what is still
> pending. Single source of truth; per-game notes live in the
> `screens/SCREEN_GAME_*.md` files. Planning history: `.local/ranking_planning.md`.

## Table of Contents

- [Status](#status)
- [Model](#model)
  - [Storage](#storage)
  - [Timeline score](#timeline-score)
- [GUI flow](#gui-flow)
- [Turning the ranking off (per installation)](#turning-the-ranking-off-per-installation)
- [Deleting rankings (admin only)](#deleting-rankings-admin-only)
- [Online ranking (implemented, pending deployment)](#online-ranking-implemented-pending-deployment)
  - [Privacy and AppsEdu](#privacy-and-appsedu)
- [Known limits](#known-limits)
- [Tests](#tests)

---

## Status

| Part | Status |
|------|--------|
| Single global ranking, local (core `RankingStore`) | ✅ Implemented |
| Local players `Player-NNN` (several per install) | ✅ Implemented |
| Zowi Says (Memory), Mouths (Pintabocas) and Timeline add to the ranking | ✅ Implemented |
| World ranking, **read** (Local / World tabs, downloads the public JSON) | ✅ Implemented; hidden until `ranking_online_read_url` is set |
| World ranking, **sharing** (checkbox, taken-number flow, delete, `server/ranking-worker/`) | ✅ Implemented and tested locally; **not deployed** until the Worker exists and the URLs are set (`.local/CLOUDFARE_HOWTO.md`) |
| Privacy policy (incl. online-ranking text) | 📝 Draft in `PRIVACY.md`, pending review |

Different from the Android original, which kept a free-text top-10 **per game**
(Zowi Says and Mouths only, no player, no global total, no Timeline).

---

## Model

- **One global ranking.** Each player's total is the sum, over the three games,
  of their **best** score in that game, normalised to points:

  `points(game) = round(100 × best / R)` with `R` = Zowi Says **12**, Mouths **8**,
  Timeline **60** (`RankingScoreConfig`). A score equal to `R` is worth 100
  points; there is no cap. The raw scores are not comparable otherwise
  (Zowi Says = sequence length − 1, Mouths = level − 1, Timeline = weighted
  complexity, see below).
- **Players.** A player is a number **100-999** shown as `Player-123`. That value
  is **both the nickname and the id**. The prefix `Player` is a fixed literal in
  every language (a "jugador" in another language would be another player).
  There is **no free text** anywhere, so real names and bad words cannot enter
  the ranking. 900 numbers are possible (000-099 are not allowed).
- **Several players per install.** One player is *active*; every finished game
  adds to the active player. The first player is created automatically (random
  free number). "New player" suggests a random free number and lets the user
  type another; if it is taken the dialog warns and proposes a free one.
- **Ties** rank the player who reached the total first.
- **Only improvements count**: a score must beat the player's stored best in that
  game; playing without improving changes nothing.

### Storage

`zowi::RankingStore` (`src/core/include/zowi/ranking_store.h`, Qt-free) keeps
everything in its own file, `ZowiRanking.json`, in the same config directory as
the session file `ZowiApp.json` (e.g. `~/.config/ZowiDesktop/`). Keys:
`players` (JSON array of `{id, zowi_says, mouths, timeline, updated}` as a string)
and `active_player`. Because it is a different file, `session clear` or any
session reset never touches the ranking. Corrupt data reads as an empty ranking;
old per-game keys from early builds are ignored (there is no migration).

### Timeline score

`timelineScore()` (`src/core/include/zowi/timeline_score.h`) gives the raw score of
a **completed** sequence: movement 3 × repetitions, gesture 2 × repetitions,
mouth 1 (ignores repetitions); movement speed Fast +20 % / Slow −20 %; +2 per
distinct item type used; only the first 40 chips count. Constants live in
`TimelineScoreConfig`. It is awarded only when the whole sequence ran with real
robot acks (`TimelineController::sequenceCompleted(score, eligible)`): Stop, a
timeout or a lost robot never score, and sequences under 5 chips are not
eligible.

## GUI flow

- `RankingController` (`Ranking` in QML): `top()`, `players()`, `activePlayer()`,
  `suggestNumber()`, `createPlayer(n)`, `setActivePlayer(n)`,
  `recordScore(game, score)`. **No delete method** by design.
- When a game ends (`GameMemoryScreen`, `GameMouthsScreen`) the screen calls
  `Ranking.recordScore(...)` and the game-over dialog shows
  *"Player-123: 150 pts · position 2"* (and *"New best!"* in Memory when the
  player's best improved). `GameTimelineScreen` records after a completed
  sequence and opens the ranking when the player improved.
- **The player is told how to score:** each game's "How to play" dialog has a
  second paragraph (key `how_to_play_ranking`, rich text with the key words in
  **bold**), separate from the game presentation, on how it adds points (Memory: a point per round, Mouths: a point per
  level, both only if the player's best improves; Timeline: play a sequence of at
  least 5 items to the end, movements > gestures > mouths, variety bonus). The
  ranking dialog has a short "every game adds your best score…" note, and a
  completed Timeline sequence with fewer than 5 items shows a message instead of
  silently not scoring (`ranking_too_short`).
- `RankingDialog.qml` (shared; i18n context `RankingDialog.qml`): the top 10
  (the active player is always shown, highlighted), "New player" and
  "Switch player". The corner **Ranking** button of the three games opens it.
  **Size:** the dialog takes `sizeRatio` (a property of `RankingDialog.qml`,
  default **0.4**) of the app window: width = 40 % of the window width (at least
  380 px), and a height that follows the content with a **minimum of 40 % of the
  window height**; it never exceeds 90 % of the window (the list then scrolls).
  To change it, edit that property or instantiate `RankingDialog { sizeRatio: 0.5 }`.

## Turning the ranking off (per installation)

The `ranking_enabled` key (default `true`) switches the whole ranking on or off
for an installation without recompiling. With `false`, **everything is hidden**
(corner buttons, ranking dialog, the ranking paragraph of the help dialogs, the
game-over total and the Timeline notice) and **nothing is recorded** (no player
is created, no score saved). Data already saved is kept and comes back when it is
turned on again; `zowi_cli ranking` still works for the administrator.

Set it in the system file, which wins over everything else:

```json
{ "ranking_enabled": "false" }
```

in `/etc/ZowiDesktop/config.json` (Windows: `%PROGRAMDATA%\ZowiDesktop\config.json`).
See `CONFIG_HOWTO.md` for the layers and the `allow_*` switches.

## Deleting rankings (admin only)

- The app has **no way** to delete players or scores: not the achievements reset
  in Settings (its text says rankings are not affected), not any other button.
- Admin, local: the CLI.

  ```bash
  zowi_cli ranking list                # every player with total and per-game bests
  zowi_cli ranking clear Player-123    # remove one player (also: clear 123)
  zowi_cli ranking clear all           # wipe the whole ranking
  ```

  Removing `ZowiRanking.json` by hand also works. `zowi_cli session clear` does
  not affect the ranking. See `ZOWI_CLI_HOWTO.md`.
- Online (future): only the maintainer, editing the published JSON on `gh-pages`.

## Online ranking (implemented, pending deployment)

How it works in the app (World tab of the ranking dialog, only when both URLs are
configured and `ranking_online_allowed` is not `false`):

- **Checkbox "Share my score in the world ranking"** — off by default; the choice is
  kept in the session (`ranking_share_online`). Nothing is sent while it is off, and
  the app makes no request at all until the World tab is opened (or sharing was
  already on at start-up).
- Turning it on sends the active player's raw bests; every later improvement of the
  total is sent again. The server computes the total and answers with a secret code
  on the first registration (kept in `ZowiRanking.json`, owner-only).
- **Number taken online** → the dialog says so and offers **Change**: the local
  player gets a free number (scores kept) and the score is sent again.
- **Not enough yet** (no points, or below the top-100 cut) → a friendly message; the
  local ranking is unaffected.
- **Delete my entry now** (shown once registered) removes the online entry at once
  and turns sharing off. Unchecking the box only stops sending; the entry expires
  after 30 days.
- Switching the active local player while sharing registers the new one; the old
  entry just expires.
- Server: `server/ranking-worker/` (Cloudflare Worker + D1). Rules and API are in its
  README; `node server/ranking-worker/test/devserver.js` runs a local stand-in
  (`ranking_online_read_url` = `http://127.0.0.1:8787/ranking.json`,
  `ranking_online_submit_url` = `http://127.0.0.1:8787`) for trying the app.

Design notes:

- **Only the top 100** is kept online. A player whose total does not enter the
  top 100 is not registered online (the local ranking still has them). With
  fewer than 100 players anyone with a total above 0 enters. Players displaced
  from the top 100 are removed and their number is freed.
- **The first to register a number wins.** If it is already online, the server
  answers "taken", the app informs the user and suggests choosing another
  number; accepting renames the local player (scores kept) and resubmits. The
  Worker serialises writes so two simultaneous registrations cannot both win.
- **Two-layer check:** the app reads the public JSON (top 100 and the 100th
  total), decides locally whether the player beats the cut and only suggests free
  numbers; the server is the authority.
- **Ownership token:** the server returns a random secret at registration; it is
  stored only locally and authorises later updates. It is never published. If it
  is lost (reinstall) the number stays taken until the player drops out of the
  top 100, expires or the admin frees it.
- **30-day expiry:** each online entry keeps its last change; with no change in 30
  days it expires and the number is freed. Only an **improvement of the total**
  counts as a change (playing without improving sends nothing). The static JSON
  is filtered by the client when read and cleaned by the Worker (on writes and a
  daily scheduled task). The `gh-pages` git history keeps old entries
  (`Player-NNN` + total, no personal data); say so in the privacy policy.
- **Abuse:** registering requires a total above the top-100 cut (validated with
  plausible per-game caps) and a per-IP daily limit.
- **Infrastructure beyond GitHub:** GitHub Pages is read-only and a token cannot
  ship in the app, so writes go through a small serverless proxy (e.g. a
  Cloudflare Worker) that holds the GitHub token.
- Opt-in, only the `Player-NNN` and the normalised totals are sent (no times, no
  history, no other data).

### Privacy and AppsEdu

Public sources of AppsEdu (Conselleria d'Educació) list: no advertising, no
payments, no student profiling, no risk to the Conselleria's security or
reputation, cybersecurity tests, and no personal student data in apps. Design
consequences: no free text (nothing personal can be typed), the id is a random
number that does not identify a person (but is a persistent pseudonym once
online → opt-in and covered in `PRIVACY.md` §6), 30-day retention online,
and nothing is sent until the user opts in. To be confirmed with the AppsEdu
channel (GVA SAI) and the Conselleria's data-protection office before publishing
the online ranking. This is not legal advice.

## Known limits

- Scores are produced by the client, so a local ranking can be edited by anyone
  with access to the file. Not a security feature.
- 900 possible numbers; online only 100 slots at a time.
- No achievements layer yet ("Delete achievements" in Settings is a placeholder).

## Tests

- Core: `test_ranking_store`, `test_timeline_score`
  (`ctest --test-dir build -R 'ranking|timeline_score'`).
- Manual: play the three games with two players, switch players from the ranking
  dialog, then check `zowi_cli ranking list`.
