# Remote worker — add a machine's CPU to the arena

The production server has only 2 cores, so big round-robins queue slowly. This
agent lets any machine (e.g. your laptop) **pull pending games, run them locally
with `unswbc`, and post results back**. The engine is deterministic, so a game
run here is identical to one run on the server — it's pure extra throughput.

The server keeps running its own 2-core worker at the same time; games are claimed
atomically, so nothing runs twice.

## Requirements

- `unswbc` on PATH, **same version as the server** (`0.3.6`) — check: `unswbc --version`
  (install: `uv tool install unswbc` or `pip install unswbc`).
- A C++ toolchain (`g++`/`clang++`) **only if you want to run C++ bots**; Python bots
  need nothing extra.
- Python 3.9+ (standard library only — no `pip install` for the agent itself).

## Run

```bash
ARENA_USER=admin ARENA_PASS='the-site-password' \
    python3 deploy/remote-worker/agent.py --concurrency 6
```

- `--url` defaults to `http://arena.peop1e.com` (the arena is HTTP-only, port 80).
- `--concurrency` = how many games to run in parallel (default 6; your Mac has 10
  cores, but leave headroom for daily use).
- Credentials are the site's nginx basic-auth (same as logging into the web UI).
- Stop with **Ctrl-C** — any game still mid-run is automatically requeued on the
  server after `BC_STALE_CLAIM_SEC` (default 15 min).

On first start it generates the map set locally (`unswbc maps`) and caches each bot
version under `~/.battlecode-arena-worker/` (versions are immutable, so they're
downloaded once).

## Env vars

| Var | Default | Meaning |
| --- | --- | --- |
| `ARENA_URL` | `http://arena.peop1e.com` | Server base URL |
| `ARENA_USER` / `ARENA_PASS` | — | Basic-auth credentials (required) |
| `BC_CONCURRENCY` | `6` | Parallel games (or `--concurrency`) |
| `BC_UNSWBC` | `unswbc` | Path to the toolkit binary |
| `BC_GAME_TIMEOUT` | `300` | Per-game timeout (seconds) |
| `BC_WORKER_DIR` | `~/.battlecode-arena-worker` | Cache dir (maps + bot versions) |
