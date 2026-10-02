// Pure rules of the world ranking (no I/O), shared by the Worker and the tests.
// Constants must match the app (RankingScoreConfig / timelineMaxPlausibleScore).

export const REFERENCE = { zowi_says: 12, mouths: 8, timeline: 60 };
export const MAX_RAW = { zowi_says: 100, mouths: 100, timeline: 726 };
export const MAX_ENTRIES = 100;
export const EXPIRY_SECONDS = 30 * 24 * 3600;
export const DAILY_LIMIT = 60;

export function points(game, raw) {
  if (raw <= 0) return 0;
  const ref = REFERENCE[game];
  return Math.floor((100 * raw + Math.floor(ref / 2)) / ref);
}

export function totalFor(b) {
  return points("zowi_says", b.zowi_says) + points("mouths", b.mouths) + points("timeline", b.timeline);
}

// Returns { ok: true, value } or { ok: false, error }.
export function validateSubmit(body) {
  if (!body || typeof body !== "object") return { ok: false, error: "invalid" };
  const { number, token } = body;
  if (!Number.isInteger(number) || number < 100 || number > 999) return { ok: false, error: "invalid" };
  if (token !== undefined && (typeof token !== "string" || token.length < 16 || token.length > 200)) {
    return { ok: false, error: "invalid" };
  }
  const bests = {};
  for (const game of Object.keys(REFERENCE)) {
    const v = body[game];
    if (!Number.isInteger(v) || v < 0 || v > MAX_RAW[game]) return { ok: false, error: "invalid" };
    bests[game] = v;
  }
  return { ok: true, value: { number, token, ...bests, total: totalFor(bests) } };
}

export function validateDelete(body) {
  if (!body || typeof body !== "object") return { ok: false, error: "invalid" };
  if (!Number.isInteger(body.number) || body.number < 100 || body.number > 999) return { ok: false, error: "invalid" };
  if (typeof body.token !== "string" || body.token.length < 16 || body.token.length > 200) {
    return { ok: false, error: "invalid" };
  }
  return { ok: true, value: { number: body.number, token: body.token } };
}

// Public document: best first (total desc, earlier update first, number asc).
export function buildPublicDocument(rows, nowSeconds) {
  const sorted = [...rows].sort(
    (a, b) => b.total - a.total || a.updated_at - b.updated_at || a.number - b.number
  );
  const players = sorted.slice(0, MAX_ENTRIES).map((r) => ({
    number: r.number,
    total: r.total,
    zowi_says: points("zowi_says", r.zowi_says),
    mouths: points("mouths", r.mouths),
    timeline: points("timeline", r.timeline),
  }));
  return { generated: new Date(nowSeconds * 1000).toISOString(), players };
}

export function randomToken() {
  const bytes = new Uint8Array(32);
  crypto.getRandomValues(bytes);
  return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

export async function sha256Hex(text) {
  const data = new TextEncoder().encode(text);
  const hash = await crypto.subtle.digest("SHA-256", data);
  return Array.from(new Uint8Array(hash), (b) => b.toString(16).padStart(2, "0")).join("");
}

export function timingSafeEqual(a, b) {
  if (a.length !== b.length) return false;
  let diff = 0;
  for (let i = 0; i < a.length; i++) diff |= a.charCodeAt(i) ^ b.charCodeAt(i);
  return diff === 0;
}
