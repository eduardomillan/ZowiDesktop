// Publishes ranking.json through the GitHub Contents API. Only ever touches
// the single configured path (docs/ranking/ranking.json).
function toBase64(text) {
  const bytes = new TextEncoder().encode(text);
  let bin = "";
  for (const b of bytes) bin += String.fromCharCode(b);
  return btoa(bin);
}

export async function publish(env, doc, fetchFn = fetch) {
  const url = `https://api.github.com/repos/${env.GITHUB_REPO}/contents/${env.GITHUB_PATH}`;
  const headers = {
    Authorization: `Bearer ${env.GITHUB_TOKEN}`,
    Accept: "application/vnd.github+json",
    "User-Agent": "zowi-ranking-worker",
    "X-GitHub-Api-Version": "2022-11-28",
  };
  const content = toBase64(JSON.stringify(doc, null, 1) + "\n");

  for (let attempt = 0; attempt < 2; attempt++) {
    let sha;
    const cur = await fetchFn(`${url}?ref=${encodeURIComponent(env.GITHUB_BRANCH)}`, { headers });
    if (cur.status === 200) sha = (await cur.json()).sha;
    else if (cur.status !== 404) { console.error("github GET", cur.status, (await cur.text()).slice(0, 200)); return false; }

    const body = { message: "Update world ranking", content, branch: env.GITHUB_BRANCH };
    if (sha) body.sha = sha;
    const put = await fetchFn(url, { method: "PUT", headers, body: JSON.stringify(body) });
    if (put.ok) return true;
    console.error("github PUT", put.status, (await put.text()).slice(0, 200));
    if (put.status !== 409 && put.status !== 422) return false;  // retry only on a stale sha
  }
  return false;
}
