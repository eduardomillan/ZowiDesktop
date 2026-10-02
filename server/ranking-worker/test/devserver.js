// Local stand-in for the deployed Worker, for trying the app without Cloudflare:
//   node test/devserver.js [port]
// Same rules as the Worker (src/service.js) over an in-memory repository.
// GET /ranking.json serves the public document; POST /submit and /delete work as in production.
import http from "node:http";
import { MemRepo } from "./memrepo.js";
import { allowRequest, remove, submit } from "../src/service.js";
import { buildPublicDocument, validateDelete, validateSubmit } from "../src/ranking.js";

const repo = new MemRepo();
const port = Number(process.argv[2]) || 8787;

http.createServer(async (req, res) => {
  const send = (status, body) => { res.writeHead(status, { "Content-Type": "application/json" }); res.end(JSON.stringify(body)); };
  const now = Math.floor(Date.now() / 1000);
  if (req.method === "GET" && req.url === "/ranking.json") return send(200, buildPublicDocument(await repo.allPlayers(), now));
  if (req.method === "GET" && req.url === "/health") return send(200, { ok: true });
  if (req.method !== "POST" || !["/submit", "/delete"].includes(req.url)) return send(404, { error: "not_found" });

  let text = "";
  for await (const chunk of req) text += chunk;
  let body; try { body = JSON.parse(text); } catch { return send(400, { error: "invalid" }); }
  const parsed = req.url === "/submit" ? validateSubmit(body) : validateDelete(body);
  if (!parsed.ok) return send(400, { error: parsed.error });
  if (!(await allowRequest(repo, "local", now))) return send(429, { error: "rate_limited" });
  const r = req.url === "/submit" ? await submit(repo, parsed.value, now) : await remove(repo, parsed.value, now);
  console.log(req.method, req.url, body.number, "->", r.status);
  send(r.status, r.body);
}).listen(port, "127.0.0.1", () => console.log(`dev server on http://127.0.0.1:${port}`));
