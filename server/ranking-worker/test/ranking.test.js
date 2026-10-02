import test from "node:test";
import assert from "node:assert/strict";
import { MemRepo } from "./memrepo.js";
import { allowRequest, remove, submit } from "../src/service.js";
import {
  DAILY_LIMIT, EXPIRY_SECONDS, buildPublicDocument, points, totalFor, validateDelete, validateSubmit,
} from "../src/ranking.js";
import { publish } from "../src/github.js";

const NOW = 1_800_000_000;
const sub = (number, zs = 0, mo = 0, tl = 0, token) => ({
  number, zowi_says: zs, mouths: mo, timeline: tl, token, total: totalFor({ zowi_says: zs, mouths: mo, timeline: tl }),
});

test("points match the app (reference 12/8/60, rounded)", () => {
  assert.equal(points("zowi_says", 12), 100);
  assert.equal(points("mouths", 4), 50);
  assert.equal(points("timeline", 30), 50);
  assert.equal(points("timeline", 0), 0);
  assert.equal(totalFor({ zowi_says: 12, mouths: 8, timeline: 60 }), 300);
});

test("validation rejects bad numbers, types and absurd scores", () => {
  const ok = { number: 123, zowi_says: 3, mouths: 2, timeline: 40 };
  assert.equal(validateSubmit(ok).ok, true);
  assert.equal(validateSubmit(ok).value.total, points("zowi_says", 3) + points("mouths", 2) + points("timeline", 40));
  for (const bad of [
    { ...ok, number: 99 }, { ...ok, number: 1000 }, { ...ok, number: "123" }, { ...ok, zowi_says: -1 },
    { ...ok, mouths: 1.5 }, { ...ok, timeline: 727 }, { ...ok, zowi_says: 101 }, { ...ok, token: 5 }, null, "x",
  ]) assert.equal(validateSubmit(bad).ok, false, JSON.stringify(bad));
  assert.equal(validateDelete({ number: 123, token: "a".repeat(64) }).ok, true);
  assert.equal(validateDelete({ number: 123 }).ok, false);
});

test("first registration returns a token; first wins on a taken number", async () => {
  const repo = new MemRepo();
  const a = await submit(repo, sub(123, 5), NOW);
  assert.equal(a.status, 200);
  assert.match(a.body.token, /^[0-9a-f]{64}$/);
  assert.equal(a.body.position, 1);
  assert.notEqual([...repo.players.values()][0].token_hash, a.body.token);  // only the hash is stored

  const b = await submit(repo, sub(123, 9), NOW + 1);
  assert.equal(b.status, 409);
  assert.equal((await repo.getPlayer(123)).zowi_says, 5);
});

test("owner updates only raise the total and renew expiry only then", async () => {
  const repo = new MemRepo();
  const { body } = await submit(repo, sub(150, 4), NOW);
  const same = await submit(repo, sub(150, 3, 0, 0, body.token), NOW + 100);
  assert.equal(same.status, 200);
  assert.equal((await repo.getPlayer(150)).updated_at, NOW);  // no increase: not renewed

  const up = await submit(repo, sub(150, 6, 0, 0, body.token), NOW + 200);
  assert.equal(up.status, 200);
  const p = await repo.getPlayer(150);
  assert.equal(p.updated_at, NOW + 200);
  assert.equal(p.zowi_says, 6);
  assert.equal(up.body.total, points("zowi_says", 6));
});

test("wrong token is unauthorized, no token on a taken number is taken", async () => {
  const repo = new MemRepo();
  await submit(repo, sub(200, 4), NOW);
  assert.equal((await submit(repo, sub(200, 5, 0, 0, "f".repeat(64)), NOW)).status, 401);
  assert.equal((await submit(repo, sub(200, 5), NOW)).status, 409);
});

test("zero total does not qualify", async () => {
  assert.equal((await submit(new MemRepo(), sub(300), NOW)).status, 422);
});

test("top 100: full list rejects non-beating totals and drops the worst", async () => {
  const repo = new MemRepo();
  for (let i = 0; i < 100; i++) await submit(repo, sub(100 + i, 1 + (i % 10)), NOW + i);  // totals 8..83
  assert.equal(repo.players.size, 100);

  const low = await submit(repo, sub(500, 1), NOW + 500);  // total 8 == cutoff
  assert.equal(low.status, 422);

  const high = await submit(repo, sub(501, 12, 8, 60), NOW + 501);
  assert.equal(high.status, 200);
  assert.equal(high.body.position, 1);
  assert.equal(repo.players.size, 100);
  const worst = (await repo.allPlayers()).reduce((m, p) => Math.min(m, p.total), 1e9);
  assert.ok(worst >= 8);
});

test("a player pushed out frees the number", async () => {
  const repo = new MemRepo();
  for (let i = 0; i < 100; i++) await submit(repo, sub(100 + i, 1), NOW + i);  // all total 8, #99 is last by time
  const r = await submit(repo, sub(900, 12), NOW + 1000);
  assert.equal(r.status, 200);
  assert.equal(await repo.getPlayer(199), null);  // latest of the ties removed
  assert.equal((await submit(repo, sub(199, 12), NOW + 1001)).status, 200);  // number reusable
});

test("entries expire after 30 days and their number is free again", async () => {
  const repo = new MemRepo();
  await submit(repo, sub(250, 4), NOW);
  const later = NOW + EXPIRY_SECONDS + 1;
  const r = await submit(repo, sub(250, 2), later);  // someone else takes it
  assert.equal(r.status, 200);
  assert.ok(r.body.token);
});

test("delete needs the token and is idempotent", async () => {
  const repo = new MemRepo();
  const { body } = await submit(repo, sub(321, 4), NOW);
  assert.equal((await remove(repo, { number: 321, token: "0".repeat(64) }, NOW)).status, 401);
  assert.equal(repo.players.size, 1);
  assert.deepEqual((await remove(repo, { number: 321, token: body.token }, NOW)).body, { deleted: true });
  assert.equal(repo.players.size, 0);
  assert.deepEqual((await remove(repo, { number: 321, token: body.token }, NOW)).body, { deleted: false });
});

test("rate limit applies per IP and day", async () => {
  const repo = new MemRepo();
  for (let i = 0; i < DAILY_LIMIT; i++) assert.equal(await allowRequest(repo, "ip1", NOW), true);
  assert.equal(await allowRequest(repo, "ip1", NOW), false);
  assert.equal(await allowRequest(repo, "ip2", NOW), true);
  assert.equal(await allowRequest(repo, "ip1", NOW + 86400), true);
});

test("public document: sorted, capped at 100, without secrets", () => {
  const rows = Array.from({ length: 120 }, (_, i) => ({
    number: 100 + i, zowi_says: 1, mouths: 0, timeline: 0, total: 8 + i, updated_at: i, token_hash: "secret",
  }));
  const doc = buildPublicDocument(rows, NOW);
  assert.equal(doc.players.length, 100);
  assert.equal(doc.players[0].number, 219);
  assert.deepEqual(Object.keys(doc.players[0]).sort(), ["mouths", "number", "timeline", "total", "zowi_says"]);
  assert.ok(!JSON.stringify(doc).includes("secret"));
});

test("publish updates the single configured path and retries a stale sha", async () => {
  const env = { GITHUB_REPO: "o/r", GITHUB_BRANCH: "gh-pages", GITHUB_PATH: "docs/ranking/ranking.json", GITHUB_TOKEN: "t" };
  const calls = [];
  let puts = 0;
  const fetchFn = async (url, opts = {}) => {
    calls.push([opts.method || "GET", url]);
    if (!opts.method) return new Response(JSON.stringify({ sha: "abc" }), { status: 200 });
    puts++;
    return new Response("{}", { status: puts === 1 ? 409 : 200 });
  };
  assert.equal(await publish(env, { players: [] }, fetchFn), true);
  assert.equal(puts, 2);
  for (const [, url] of calls) assert.ok(url.startsWith("https://api.github.com/repos/o/r/contents/docs/ranking/ranking.json"));
});
