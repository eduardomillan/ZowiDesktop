// Ranking rules on top of an abstract repository (D1 in production, an
// in-memory one in the tests). Keeps the HTTP layer and the SQL out of the logic.
import {
  EXPIRY_SECONDS, MAX_ENTRIES, DAILY_LIMIT, totalFor, randomToken, sha256Hex, timingSafeEqual,
} from "./ranking.js";

const byRank = (a, b) => b.total - a.total || a.updated_at - b.updated_at || a.number - b.number;

function positionOf(rows, number) {
  return [...rows].sort(byRank).findIndex((r) => r.number === number) + 1;
}

// Deletes entries not improved for 30 days; returns how many were removed.
export async function expire(repo, now) {
  const removed = await repo.deleteOlderThan(now - EXPIRY_SECONDS);
  if (removed > 0) await repo.setMeta("dirty", "1");
  return removed;
}

// Keeps only the best MAX_ENTRIES players.
export async function trim(repo) {
  const rows = (await repo.allPlayers()).sort(byRank);
  const extra = rows.slice(MAX_ENTRIES);
  for (const r of extra) await repo.deletePlayer(r.number);
  return extra.length;
}

// Returns true while the client is within its daily allowance.
export async function allowRequest(repo, ipHash, now) {
  const day = new Date(now * 1000).toISOString().slice(0, 10);
  const count = await repo.bumpRate(ipHash, day);
  return count <= DAILY_LIMIT;
}

export async function submit(repo, v, now) {
  await expire(repo, now);
  const existing = await repo.getPlayer(v.number);

  if (existing) {
    if (!v.token) return { status: 409, body: { error: "taken" } };
    const ok = timingSafeEqual(await sha256Hex(v.token), existing.token_hash);
    if (!ok) return { status: 401, body: { error: "unauthorized" } };

    const merged = {
      zowi_says: Math.max(existing.zowi_says, v.zowi_says),
      mouths: Math.max(existing.mouths, v.mouths),
      timeline: Math.max(existing.timeline, v.timeline),
    };
    const total = totalFor(merged);
    if (total > existing.total) {  // only an increase renews the 30 days
      await repo.updatePlayer(v.number, { ...merged, total, updated_at: now });
      await repo.setMeta("dirty", "1");
    }
    const rows = await repo.allPlayers();
    return { status: 200, body: { total: Math.max(total, existing.total), position: positionOf(rows, v.number) } };
  }

  if (v.total <= 0) return { status: 422, body: { error: "not_qualified" } };
  const rows = await repo.allPlayers();
  if (rows.length >= MAX_ENTRIES) {
    const cutoff = [...rows].sort(byRank)[MAX_ENTRIES - 1].total;
    if (v.total <= cutoff) return { status: 422, body: { error: "not_qualified" } };
  }

  const token = randomToken();
  const created = await repo.insertPlayer({
    number: v.number, zowi_says: v.zowi_says, mouths: v.mouths, timeline: v.timeline,
    total: v.total, updated_at: now, created_at: now, token_hash: await sha256Hex(token),
  });
  if (!created) return { status: 409, body: { error: "taken" } };  // lost a race: first wins

  await trim(repo);
  await repo.setMeta("dirty", "1");
  const all = await repo.allPlayers();
  return { status: 200, body: { token, total: v.total, position: positionOf(all, v.number) } };
}

export async function remove(repo, v, now) {
  await expire(repo, now);
  const existing = await repo.getPlayer(v.number);
  if (!existing) return { status: 200, body: { deleted: false } };  // nothing to delete: idempotent
  const ok = timingSafeEqual(await sha256Hex(v.token), existing.token_hash);
  if (!ok) return { status: 401, body: { error: "unauthorized" } };
  await repo.deletePlayer(v.number);
  await repo.setMeta("dirty", "1");
  return { status: 200, body: { deleted: true } };
}
