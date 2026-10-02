# Zowi world-ranking Worker

Cloudflare Worker + D1 behind the optional world ranking of Zowi Desktop. The app
never holds a GitHub token: it talks to this Worker, which validates scores and
publishes `docs/ranking/ranking.json` on the `gh-pages` branch (read by the app as a
static file). Step-by-step setup for non-experts: `.local/CLOUDFARE_HOWTO.md`.

## Rules

- Player numbers are 100-999 (`Player-NNN`). **The first to register a number owns it**
  (primary key in D1). Registration returns a random 256-bit token once; only its
  SHA-256 hash is stored.
- The client sends raw bests (`zowi_says`, `mouths`, `timeline`); the server computes
  the total (references 12 / 8 / 60, 100 points each) and rejects implausible values
  (caps 100 / 100 / 726 — keep in sync with the app).
- Only the best 100 are kept. A new player must beat the 100th total; whoever drops
  out is removed and the number is freed.
- Entries expire 30 days after their last **increase** of the total.
- Rate limit: 60 requests per IP (hashed) per day.
- Publication: at most once per 60 s after a change; a cron every 5 min publishes
  pending changes and expires old entries. Only `docs/ranking/ranking.json` is touched.

## API (JSON, POST)

| Endpoint | Body | Replies |
|---|---|---|
| `/submit` | `{number, zowi_says, mouths, timeline, token?}` | `200 {token?, total, position}` · `409` number taken · `422` not qualified · `401` wrong token · `429` rate limit · `400` invalid |
| `/delete` | `{number, token}` | `200 {deleted}` · `401` · `429` · `400` |
| `GET /health` | | `200 {"ok":true}` |

`token` is omitted on the first registration and sent on every later update.

## Develop and test

```bash
cd server/ranking-worker
npm test                 # node --test, in-memory repository
npx wrangler dev         # local Worker with a local D1 (apply schema.sql first)
npx wrangler d1 execute zowi-ranking --local --file=schema.sql
```

## Deploy

See `.local/CLOUDFARE_HOWTO.md` (part D): `wrangler d1 create`, put the id in
`wrangler.toml`, apply `schema.sql` with `--remote`, `wrangler secret put GITHUB_TOKEN`,
`wrangler deploy`.
