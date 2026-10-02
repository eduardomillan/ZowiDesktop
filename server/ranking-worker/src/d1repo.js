// Repository over Cloudflare D1. The primary key makes "first wins" atomic.
export class D1Repo {
  constructor(db) { this.db = db; }

  async getPlayer(number) {
    return (await this.db.prepare("SELECT * FROM players WHERE number = ?").bind(number).first()) ?? null;
  }
  async allPlayers() {
    return (await this.db.prepare("SELECT * FROM players").all()).results;
  }
  async insertPlayer(p) {
    const r = await this.db.prepare(
      `INSERT INTO players (number, zowi_says, mouths, timeline, total, updated_at, created_at, token_hash)
       VALUES (?, ?, ?, ?, ?, ?, ?, ?) ON CONFLICT(number) DO NOTHING`
    ).bind(p.number, p.zowi_says, p.mouths, p.timeline, p.total, p.updated_at, p.created_at, p.token_hash).run();
    return r.meta.changes === 1;
  }
  async updatePlayer(number, f) {
    await this.db.prepare(
      "UPDATE players SET zowi_says=?, mouths=?, timeline=?, total=?, updated_at=? WHERE number=?"
    ).bind(f.zowi_says, f.mouths, f.timeline, f.total, f.updated_at, number).run();
  }
  async deletePlayer(number) {
    await this.db.prepare("DELETE FROM players WHERE number = ?").bind(number).run();
  }
  async deleteOlderThan(cutoff) {
    const r = await this.db.prepare("DELETE FROM players WHERE updated_at < ?").bind(cutoff).run();
    return r.meta.changes;
  }
  async bumpRate(ipHash, day) {
    await this.db.prepare(
      `INSERT INTO rate_limit (ip_hash, day, count) VALUES (?, ?, 1)
       ON CONFLICT(ip_hash, day) DO UPDATE SET count = count + 1`
    ).bind(ipHash, day).run();
    const row = await this.db.prepare("SELECT count FROM rate_limit WHERE ip_hash=? AND day=?").bind(ipHash, day).first();
    return row.count;
  }
  async purgeRate(beforeDay) {
    await this.db.prepare("DELETE FROM rate_limit WHERE day < ?").bind(beforeDay).run();
  }
  async getMeta(key) {
    const row = await this.db.prepare("SELECT value FROM meta WHERE key = ?").bind(key).first();
    return row ? row.value : null;
  }
  async setMeta(key, value) {
    await this.db.prepare(
      "INSERT INTO meta (key, value) VALUES (?, ?) ON CONFLICT(key) DO UPDATE SET value = excluded.value"
    ).bind(key, value).run();
  }
}
