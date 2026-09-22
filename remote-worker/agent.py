#!/usr/bin/env python3
"""Battlecode Arena — remote worker agent.

Adds a machine's CPU to the arena's match pool. It pulls pending games from the
server, runs them locally with `unswbc`, and posts the result (+ replay) back.
The engine is deterministic, so a game run here is identical to one run on the
server — this is pure extra throughput, no correctness risk.

Requirements on this machine:
  * `unswbc` on PATH, SAME version as the server (0.3.6) for reproducibility
  * a C++ toolchain (g++/clang++) if you want to run C++ bots; Python bots need nothing
  * Python 3.9+ (stdlib only — no pip installs)

Run:
  ARENA_USER=admin ARENA_PASS='...' \
      python3 agent.py --concurrency 6

Everything talks to the server over the same nginx basic-auth as the website.
Stop with Ctrl-C; any game left mid-run is automatically requeued server-side.
"""

from __future__ import annotations

import argparse
import base64
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import urllib.error
import urllib.request
import zipfile
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

# Same verdict format the server parses (see backend/app/runner/runner.py).
_RESULT_RE = re.compile(
    r"^(?:team (?P<team>[AB]) wins|(?P<draw>draw)) after (?P<rounds>\d+) rounds \((?P<reason>.+?)\)",
    re.MULTILINE,
)


def parse_result(stdout: str):
    """Return (winner, rounds, reason) with winner 'A'|'B'|'draw', or None."""
    m = None
    for m in _RESULT_RE.finditer(stdout):
        pass
    if m is None:
        return None
    return ("draw" if m.group("draw") else m.group("team")), int(m.group("rounds")), m.group("reason")


def clean_log(raw: str) -> str:
    lines = []
    for line in raw.replace("\r", "\n").splitlines():
        s = line.strip()
        if not s or s.startswith("running round"):
            continue
        lines.append(s)
    return "\n".join(lines)[-20000:]


class Arena:
    """Tiny HTTP client for the arena API with preemptive Basic auth."""

    def __init__(self, base_url: str, user: str, password: str):
        self.base = base_url.rstrip("/")
        self._auth = "Basic " + base64.b64encode(f"{user}:{password}".encode()).decode()

    def _req(self, method: str, path: str, *, body: bytes | None = None,
             content_type: str | None = None, timeout: int = 60):
        req = urllib.request.Request(self.base + path, data=body, method=method)
        req.add_header("Authorization", self._auth)
        if content_type:
            req.add_header("Content-Type", content_type)
        return urllib.request.urlopen(req, timeout=timeout)

    def claim(self):
        with self._req("POST", "/api/worker/claim", body=b"", timeout=30) as r:
            return json.loads(r.read() or b"{}")

    def download_version(self, version_id: int) -> bytes:
        with self._req("GET", f"/api/versions/{version_id}/download", timeout=120) as r:
            return r.read()

    def download_map(self, name: str) -> bytes:
        with self._req("GET", f"/api/maps/{name}/download", timeout=60) as r:
            return r.read()

    def submit(self, game_id: int, fields: dict, replay: bytes | None):
        body, ctype = _multipart(fields, {"replay": (f"{game_id}.replay", replay)} if replay else {})
        with self._req("POST", f"/api/worker/submit/{game_id}", body=body,
                       content_type=ctype, timeout=120) as r:
            return json.loads(r.read() or b"{}")


def _multipart(fields: dict, files: dict):
    boundary = "----bcworker" + os.urandom(12).hex()
    b = boundary.encode()
    out = io.BytesIO()
    for name, val in fields.items():
        if val is None:
            continue
        out.write(b"--" + b + b"\r\n")
        out.write(f'Content-Disposition: form-data; name="{name}"\r\n\r\n'.encode())
        out.write(str(val).encode() + b"\r\n")
    for name, (fname, data) in files.items():
        out.write(b"--" + b + b"\r\n")
        out.write(f'Content-Disposition: form-data; name="{name}"; filename="{fname}"\r\n'.encode())
        out.write(b"Content-Type: application/octet-stream\r\n\r\n")
        out.write(data + b"\r\n")
    out.write(b"--" + b + b"--\r\n")
    return out.getvalue(), f"multipart/form-data; boundary={boundary}"


class Worker:
    def __init__(self, args):
        self.args = args
        self.arena = Arena(args.url, args.user, args.password)
        self.workdir = Path(args.workdir).expanduser()
        self.versions_dir = self.workdir / "versions"
        self.maps_dir = self.workdir / "maps"
        self.versions_dir.mkdir(parents=True, exist_ok=True)
        self.maps_dir.mkdir(parents=True, exist_ok=True)
        self._version_locks: dict[int, threading.Lock] = {}
        self._map_locks: dict[str, threading.Lock] = {}
        self._locks_guard = threading.Lock()
        self._stop = threading.Event()
        self._done = 0
        self._counter_lock = threading.Lock()

    # --- setup ---------------------------------------------------------------
    def ensure_map(self, name: str) -> Path:
        """Fetch a map from the server on demand (official + extended), cache it."""
        dest = self.maps_dir / f"{name}.map"
        with self._locks_guard:
            lock = self._map_locks.setdefault(name, threading.Lock())
        with lock:
            if not dest.exists():
                dest.write_bytes(self.arena.download_map(name))
            return dest

    def _version_lock(self, vid: int) -> threading.Lock:
        with self._locks_guard:
            return self._version_locks.setdefault(vid, threading.Lock())

    def project_dir(self, version_id: int) -> Path:
        """Return a cached, ready-to-run project dir for a version (download once)."""
        dest = self.versions_dir / str(version_id)
        with self._version_lock(version_id):
            if dest.exists() and any(dest.iterdir()):
                return _project_root(dest)
            tmp = dest.with_suffix(".tmp")
            if tmp.exists():
                shutil.rmtree(tmp)
            tmp.mkdir(parents=True)
            data = self.arena.download_version(version_id)
            with zipfile.ZipFile(io.BytesIO(data)) as zf:
                zf.extractall(tmp)
            if dest.exists():
                shutil.rmtree(dest)
            tmp.rename(dest)
            return _project_root(dest)

    # --- run one game --------------------------------------------------------
    def run_game(self, g: dict):
        gid = g["game_id"]
        try:
            map_path = self.ensure_map(g["map_name"])
        except Exception as exc:  # download failure
            return self._submit_error(gid, f"fetch map failed: {g['map_name']}: {exc}")
        try:
            src_a = self.project_dir(g["version_a_id"])
            src_b = self.project_dir(g["version_b_id"])
        except Exception as exc:  # download/unzip failure
            return self._submit_error(gid, f"fetch project failed: {exc}")

        with tempfile.TemporaryDirectory(prefix="bc_game_") as tmp:
            a_dir, b_dir = Path(tmp) / "a", Path(tmp) / "b"
            replay = Path(tmp) / f"{gid}.replay"
            shutil.copytree(src_a, a_dir)
            shutil.copytree(src_b, b_dir)
            cmd = [self.args.unswbc, "run", str(map_path), str(a_dir), str(b_dir), "-o", str(replay)]
            start = time.monotonic()
            try:
                proc = subprocess.run(cmd, capture_output=True, text=True, timeout=self.args.timeout)
            except subprocess.TimeoutExpired as exc:
                dur = int((time.monotonic() - start) * 1000)
                return self._submit_error(gid, "timeout", clean_log((exc.stdout or "") + (exc.stderr or "")), dur)
            dur = int((time.monotonic() - start) * 1000)
            combined = (proc.stdout or "") + ("\n" + proc.stderr if proc.stderr else "")
            parsed = parse_result(proc.stdout or "")
            if parsed is None:
                reason = "no result parsed (build error?)" if proc.returncode == 0 else f"exit {proc.returncode}"
                return self._submit_error(gid, reason, clean_log(combined), dur)
            winner, rounds, reason = parsed
            replay_bytes = replay.read_bytes() if replay.exists() else None
            self.arena.submit(gid, {
                "status": "done", "winner": winner, "reason": reason,
                "rounds": rounds, "duration_ms": dur, "stdout_log": clean_log(combined),
            }, replay_bytes)
            with self._counter_lock:
                self._done += 1
            print(f"[game {gid}] {g['version_a_id']} vs {g['version_b_id']} on {g['map_name']}"
                  f" -> {winner} ({rounds}r, {dur}ms)  [total {self._done}]", flush=True)

    def _submit_error(self, gid: int, reason: str, log: str = "", dur=None):
        print(f"[game {gid}] ERROR: {reason}", flush=True)
        try:
            self.arena.submit(gid, {"status": "error", "winner": "error", "reason": reason,
                                    "rounds": None, "duration_ms": dur, "stdout_log": log}, None)
        except urllib.error.HTTPError as e:
            if e.code == 409:  # server already requeued/finished it — fine
                return
            raise

    # --- worker loop ---------------------------------------------------------
    def loop(self):
        while not self._stop.is_set():
            try:
                g = self.arena.claim()
            except urllib.error.HTTPError as e:
                print(f"[claim] HTTP {e.code} {e.reason}", flush=True)
                if e.code in (401, 403):
                    self._stop.set()  # bad credentials — no point retrying
                self._stop.wait(5)
                continue
            except Exception as exc:  # network hiccup
                print(f"[claim] {exc}", flush=True)
                self._stop.wait(5)
                continue
            if not g:
                self._stop.wait(self.args.poll_idle)
                continue
            try:
                self.run_game(g)
            except urllib.error.HTTPError as e:
                # 409 on submit = game was reclaimed elsewhere; just move on.
                print(f"[game {g.get('game_id')}] submit HTTP {e.code}", flush=True)
            except Exception as exc:
                print(f"[game {g.get('game_id')}] {exc}", flush=True)

    def run(self):
        print(f"[start] {self.args.concurrency} workers -> {self.args.url}"
              f" (cache: {self.workdir})", flush=True)
        with ThreadPoolExecutor(max_workers=self.args.concurrency, thread_name_prefix="bcw") as ex:
            for _ in range(self.args.concurrency):
                ex.submit(self.loop)
            try:
                while not self._stop.is_set():
                    time.sleep(0.5)
            except KeyboardInterrupt:
                print("\n[stop] shutting down (in-flight games will be requeued server-side)", flush=True)
                self._stop.set()


def _project_root(dest: Path) -> Path:
    """The download zip wraps the project in a single top folder; run from it."""
    entries = [p for p in dest.iterdir() if p.name not in ("__MACOSX",)]
    if len(entries) == 1 and entries[0].is_dir():
        return entries[0]
    return dest


def main():
    p = argparse.ArgumentParser(description="Battlecode Arena remote worker")
    p.add_argument("--url", default=os.environ.get("ARENA_URL", "http://arena.peop1e.com"))
    p.add_argument("--user", default=os.environ.get("ARENA_USER"))
    p.add_argument("--password", default=os.environ.get("ARENA_PASS"))
    p.add_argument("--concurrency", type=int, default=int(os.environ.get("BC_CONCURRENCY", "6")))
    p.add_argument("--unswbc", default=os.environ.get("BC_UNSWBC", "unswbc"))
    p.add_argument("--timeout", type=int, default=int(os.environ.get("BC_GAME_TIMEOUT", "300")))
    p.add_argument("--poll-idle", type=float, default=3.0, help="seconds to wait when the queue is empty")
    p.add_argument("--workdir", default=os.environ.get("BC_WORKER_DIR", "~/.battlecode-arena-worker"))
    args = p.parse_args()

    if not args.user or not args.password:
        sys.exit("error: set ARENA_USER and ARENA_PASS (the site's basic-auth credentials)")
    if not shutil.which(args.unswbc):
        sys.exit(f"error: '{args.unswbc}' not found on PATH (install: uv tool install unswbc)")

    Worker(args).run()


if __name__ == "__main__":
    main()
