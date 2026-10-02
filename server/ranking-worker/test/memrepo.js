// In-memory stand-in for D1Repo, same interface.
export class MemRepo {
  constructor() { this.players = new Map(); this.rate = new Map(); this.meta = new Map(); }
  async getPlayer(n) { const p = this.players.get(n); return p ? { ...p } : null; }
  async allPlayers() { return [...this.players.values()].map((p) => ({ ...p })); }
  async insertPlayer(p) { if (this.players.has(p.number)) return false; this.players.set(p.number, { ...p }); return true; }
  async updatePlayer(n, f) { Object.assign(this.players.get(n), f); }
  async deletePlayer(n) { this.players.delete(n); }
  async deleteOlderThan(cutoff) {
    let n = 0;
    for (const [k, p] of this.players) if (p.updated_at < cutoff) { this.players.delete(k); n++; }
    return n;
  }
  async bumpRate(ip, day) { const k = ip + day; const c = (this.rate.get(k) || 0) + 1; this.rate.set(k, c); return c; }
  async getMeta(k) { return this.meta.has(k) ? this.meta.get(k) : null; }
  async setMeta(k, v) { this.meta.set(k, v); }
}
