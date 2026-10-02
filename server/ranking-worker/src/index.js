import { D1Repo } from "./d1repo.js";
import { allowRequest, expire, remove, submit } from "./service.js";
import { buildPublicDocument, sha256Hex, validateDelete, validateSubmit } from "./ranking.js";
import { publish } from "./github.js";

const MIN_PUBLISH_INTERVAL = 60;  // seconds between publications
const MAX_BODY = 2048;

const json = (status, body) =>
  new Response(JSON.stringify(body), { status, headers: { "Content-Type": "application/json" } });

async function maybePublish(env, repo, now, force = false) {
  if ((await repo.getMeta("dirty")) !== "1") return;
  const last = Number(await repo.getMeta("last_publish")) || 0;
  if (!force && now - last < MIN_PUBLISH_INTERVAL) return;  // the cron will do it
  if (!env.GITHUB_TOKEN) return;
  const doc = buildPublicDocument(await repo.allPlayers(), now);
  if (await publish(env, doc)) {
    await repo.setMeta("dirty", "0");
    await repo.setMeta("last_publish", String(now));
  }
}

async function handle(request, env, ctx) {
  const url = new URL(request.url);
  if (url.pathname === "/health") return json(200, { ok: true });
  if (request.method !== "POST" || (url.pathname !== "/submit" && url.pathname !== "/delete")) {
    return json(404, { error: "not_found" });
  }

  const text = await request.text();
  if (text.length > MAX_BODY) return json(400, { error: "invalid" });
  let body;
  try { body = JSON.parse(text); } catch { return json(400, { error: "invalid" }); }

  const isSubmit = url.pathname === "/submit";
  const parsed = isSubmit ? validateSubmit(body) : validateDelete(body);
  if (!parsed.ok) return json(400, { error: parsed.error });

  const repo = new D1Repo(env.DB);
  const now = Math.floor(Date.now() / 1000);
  const ip = request.headers.get("CF-Connecting-IP") || "unknown";
  if (!(await allowRequest(repo, await sha256Hex(ip), now))) return json(429, { error: "rate_limited" });

  const result = isSubmit ? await submit(repo, parsed.value, now) : await remove(repo, parsed.value, now);
  ctx.waitUntil(maybePublish(env, repo, now));
  return json(result.status, result.body);
}

export default {
  async fetch(request, env, ctx) {
    try { return await handle(request, env, ctx); }
    catch (e) { console.error("unhandled", e && e.message); return json(500, { error: "server_error" }); }
  },

  // Every 5 minutes: expire old entries, drop old rate-limit rows, publish pending changes.
  async scheduled(_event, env, ctx) {
    const repo = new D1Repo(env.DB);
    const now = Math.floor(Date.now() / 1000);
    ctx.waitUntil((async () => {
      await expire(repo, now);
      await repo.purgeRate(new Date((now - 2 * 86400) * 1000).toISOString().slice(0, 10));
      await maybePublish(env, repo, now, true);
    })());
  },
};
