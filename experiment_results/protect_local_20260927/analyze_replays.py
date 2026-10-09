#!/usr/bin/env python3
"""Read packed Cap'n Proto UNSWBC replays without third-party dependencies.

The schema offsets and body-update semantics follow the official local replay
viewer bundled in unswbc/replay-viewer.vsix (Replay format versions 0..2).
Head visits count initial heads, child birth heads, and successful movement
updates, including individual sprint steps. Team repeat rate is
(head samples - team unique head cells) / head samples. This measures head
movement repetition, NOT actual explored vision or unknown-frontier coverage.
Checkpoint 50 means after rounds 0..49. A match ending sooner is not padded;
its final state is provided separately and later checkpoints are omitted.

Usage:
  python3 analyze_replays.py RUN_DIR [RUN_DIR ...] --output-prefix OUTPUT
  python3 analyze_replays.py --replay ONE.replay --bot1-side A
  python3 analyze_replays.py --compare ONE.replay TWO.replay
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import struct
import sys
from collections import Counter
from pathlib import Path

LOG_TAGS = ("PROTECT", "PROTECT_KEEP", "PROTECT_DIRECT", "PSTAT")
DEATH_REASONS = ("wall", "self", "body", "head", "invalid_action")


def unpack_packed(data: bytes) -> bytes:
    out = bytearray()
    cursor = 0
    while cursor < len(data):
        tag = data[cursor]
        cursor += 1
        word = bytearray(8)
        for bit in range(8):
            if tag & (1 << bit):
                if cursor >= len(data):
                    raise ValueError("truncated packed word")
                word[bit] = data[cursor]
                cursor += 1
        out.extend(word)
        if tag in (0, 255):
            if cursor >= len(data):
                raise ValueError("truncated packed run")
            count = data[cursor] * 8
            cursor += 1
            if tag == 0:
                out.extend(bytes(count))
            else:
                if cursor + count > len(data):
                    raise ValueError("truncated literal run")
                out.extend(data[cursor:cursor + count])
                cursor += count
    return bytes(out)


class Message:
    def __init__(self, packed: bytes):
        raw = unpack_packed(packed)
        n = struct.unpack_from("<I", raw)[0] + 1
        if not 1 <= n <= 4096:
            raise ValueError(f"invalid segment count: {n}")
        lengths = struct.unpack_from("<" + "I" * n, raw, 4)
        offset = ((4 * (n + 1) + 7) // 8) * 8
        self.segments = []
        for length in lengths:
            end = offset + length * 8
            if end > len(raw):
                raise ValueError("segment extends beyond message")
            self.segments.append(memoryview(raw)[offset:end])
            offset = end

    def word(self, segment: int, offset: int) -> int:
        return struct.unpack_from("<Q", self.segments[segment], offset)[0]

    def resolve(self, segment: int, offset: int, depth=0):
        if depth > 8:
            raise ValueError("far-pointer chain too deep")
        word = self.word(segment, offset)
        if word == 0:
            return None
        kind = word & 3
        if kind == 2:
            target_segment = word >> 32
            landing = ((word >> 3) & 0x1FFFFFFF) * 8
            if word & 4:
                far = self.word(target_segment, landing)
                tag = self.word(target_segment, landing + 8)
                return far >> 32, ((far >> 3) & 0x1FFFFFFF) * 8, tag
            return self.resolve(target_segment, landing, depth + 1)
        relative = (word >> 2) & 0x3FFFFFFF
        if relative & 0x20000000:
            relative -= 0x40000000
        return segment, offset + 8 + relative * 8, word

    def structure(self, segment: int, offset: int):
        found = self.resolve(segment, offset)
        if found is None:
            return Struct(self, 0, 0, 0, 0)
        seg, start, word = found
        if word & 3:
            raise ValueError("expected struct pointer")
        return Struct(self, seg, start, (word >> 32) & 65535, word >> 48)


class Struct:
    def __init__(self, message, segment, start, data_words, pointer_words):
        self.message, self.segment, self.start = message, segment, start
        self.data_words, self.pointer_words = data_words, pointer_words

    def number(self, offset=0, fmt="i"):
        if offset + struct.calcsize("<" + fmt) > 8 * self.data_words:
            return 0
        return struct.unpack_from("<" + fmt, self.message.segments[self.segment], self.start + offset)[0]

    def pointer(self, index):
        if index >= self.pointer_words:
            return None
        return self.message.resolve(self.segment, self.start + 8 * (self.data_words + index))

    def child(self, index):
        if index >= self.pointer_words:
            return Struct(self.message, 0, 0, 0, 0)
        return self.message.structure(self.segment, self.start + 8 * (self.data_words + index))

    def text(self, index):
        found = self.pointer(index)
        if found is None:
            return ""
        seg, start, word = found
        if word & 3 != 1 or (word >> 32) & 7 != 2:
            raise ValueError("expected text byte list")
        size = word >> 35
        return bytes(self.message.segments[seg][start:start + max(0, size - 1)]).decode("utf-8", errors="replace")

    def structs(self, index):
        found = self.pointer(index)
        if found is None:
            return
        seg, start, word = found
        if word & 3 != 1 or (word >> 32) & 7 != 7:
            raise ValueError("expected composite list")
        tag = self.message.word(seg, start)
        count = (tag & 0xFFFFFFFF) >> 2
        dw, pw = (tag >> 32) & 65535, tag >> 48
        stride = 8 * (dw + pw)
        for i in range(count):
            yield Struct(self.message, seg, start + 8 + i * stride, dw, pw)

    def uint16s(self, index):
        found = self.pointer(index)
        if found is None:
            return []
        seg, start, word = found
        if word & 3 != 1 or (word >> 32) & 7 != 3:
            raise ValueError("expected uint16 list")
        return list(struct.unpack_from("<" + "H" * (word >> 35), self.message.segments[seg], start))


def point(value):
    return value.number(0), value.number(4)


def parse_map(text):
    dragons, teams = {}, {}
    width = height = 0
    for line in text.splitlines():
        words = line.split("#", 1)[0].split()
        if not words:
            continue
        if words[0] == "MAP":
            width, height = map(int, words[1:3])
        elif words[0] == "DRAGON":
            side = "AB"[int(words[1])]
            count = int(words[2])
            xy = list(map(int, words[3:]))
            body = list(zip(xy[::2], xy[1::2]))
            if len(body) != count:
                raise ValueError("invalid initial dragon body")
            dragon_id = len(dragons)
            dragons[dragon_id] = body
            teams[dragon_id] = side
    if not width or not height or not dragons:
        raise ValueError("invalid replay map")
    return width, height, dragons, teams


def analyze_replay(path, bot1_side="A", checkpoints=(50, 100, 200), keep_trace=False):
    root = Message(Path(path).read_bytes()).structure(0, 0)
    version = root.number(0, "I")
    if version > 2:
        raise ValueError(f"unsupported replay format {version}")
    width, height, dragons, teams = parse_map(root.text(0))
    samples = Counter()
    unique = {side: set() for side in "AB"}
    logs = {side: Counter() for side in "AB"}
    deaths = {side: Counter() for side in "AB"}
    trace = []
    action_hash = {side: hashlib.sha256() for side in "AB"}
    round_num = -1
    round_count = 0
    checkpoint_rows = []
    last_snapshot = []

    def visit(dragon_id, location):
        side = teams[dragon_id]
        samples[side] += 1
        unique[side].add(location)

    for dragon_id, body in dragons.items():
        visit(dragon_id, body[0])

    def record(kind, dragon_id, detail):
        item = (round_num, kind, dragon_id, detail)
        side = teams.get(dragon_id)
        if side:
            action_hash[side].update(json.dumps(item, separators=(",", ":")).encode() + bytes([10]))
        if keep_trace:
            trace.append(item)

    def snapshot(completed, final=False):
        rows = []
        for side in "AB":
            lengths = [len(body) for dragon_id, body in dragons.items() if teams[dragon_id] == side]
            row = {"team": side, "owner": "bot1" if side == bot1_side else "bot2",
                   "checkpoint": "final" if final else completed,
                   "completed_rounds": completed, "last_round_index": round_num,
                   "head_samples": samples[side], "unique_head_cells": len(unique[side]),
                   "repeat_head_samples": samples[side] - len(unique[side]),
                   "head_repeat_rate": (samples[side] - len(unique[side])) / samples[side] if samples[side] else 0.0,
                   "population": len(lengths), "total_length": sum(lengths), "max_length": max(lengths, default=0)}
            for tag in LOG_TAGS:
                row["log_" + tag] = logs[side][tag]
            for reason in DEATH_REASONS:
                row["deaths_" + reason] = deaths[side][reason]
            rows.append(row)
        return rows

    for event in root.structs(3):
        kind = event.number(0, "H")
        data = event.child(0)
        if kind == 0:
            if round_num >= 0:
                round_count += 1
                last_snapshot = snapshot(round_count)
                if round_count in checkpoints:
                    checkpoint_rows.extend(last_snapshot)
            round_num = data.number(0)
        elif kind == 4:
            dragon_id = data.number(0)
            action = data.child(0)
            action_kind = action.number(0, "H")
            detail = action.uint16s(0) if action_kind == 0 else action.number(4) if action_kind == 1 else None
            record("action", dragon_id, [action_kind, detail])
        elif kind == 6:
            dragon_id, text = data.number(0), data.text(0)
            words = text.split()
            tag = words[1] if words and words[0] == "LOG" and len(words) > 1 else words[0] if words else ""
            if tag in LOG_TAGS and dragon_id in teams:
                logs[teams[dragon_id]][tag] += 1
        elif kind == 9:
            dragon_id = data.number(0)
            head, tail = point(data.child(0)), point(data.child(1))
            if dragon_id not in dragons:
                raise ValueError(f"update for missing dragon {dragon_id}")
            body = dragons[dragon_id]
            body.insert(0, head)
            while len(body) > 1 and body[-1] != tail:
                body.pop()
            if body[-1] != tail:
                raise ValueError(f"tail missing after update: dragon {dragon_id}")
            visit(dragon_id, head)
            record("position", dragon_id, [head, tail, len(body)])
        elif kind == 10:
            parent, child, team = data.number(0), data.number(4), "AB"[data.number(8, "H")]
            parent_body = [point(p) for p in data.structs(0)]
            child_body = [point(p) for p in data.structs(1)]
            dragons[parent], dragons[child], teams[child] = parent_body, child_body, team
            visit(child, child_body[0])
            record("split", parent, [child, parent_body, child_body])
        elif kind == 11:
            dragon_id = data.number(0)
            reason = DEATH_REASONS[data.number(4, "H")]
            if dragon_id in teams:
                deaths[teams[dragon_id]][reason] += 1
            dragons.pop(dragon_id, None)
            record("death", dragon_id, reason)
        elif kind > 12:
            raise ValueError(f"unknown event kind {kind}")
    if round_num >= 0:
        round_count += 1
        if round_count in checkpoints:
            checkpoint_rows.extend(snapshot(round_count))
    final_rows = snapshot(round_count, final=True)
    result = root.child(4)
    winner = "AB"[result.number(6, "H")] if result.number(4, "H") == 1 else "draw"
    standings = {}
    for i, side in enumerate("AB"):
        standing = result.child(i)
        actual = {"population": standing.number(0), "max_length": standing.number(4), "total_length": standing.number(8)}
        reconstructed = {key: final_rows[i][key] for key in actual}
        if actual != reconstructed:
            raise ValueError(f"final standings mismatch for {side}: replay={actual}, rebuilt={reconstructed}")
        standings[side] = actual
    return {"replay": str(Path(path).resolve()), "format_version": version, "width": width, "height": height,
            "completed_rounds": round_count, "winner": winner, "bot1_side": bot1_side,
            "checkpoints": checkpoint_rows, "final": final_rows, "standings": standings,
            "action_position_sha256": {side: value.hexdigest() for side, value in action_hash.items()},
            **({"trace": trace} if keep_trace else {})}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("run_dirs", nargs="*", type=Path)
    parser.add_argument("--replay", type=Path)
    parser.add_argument("--bot1-side", choices=["A", "B"], default="A")
    parser.add_argument("--compare", nargs=2, type=Path)
    parser.add_argument("--output-prefix", type=Path)
    args = parser.parse_args()
    if args.compare:
        first, second = (analyze_replay(path, keep_trace=True) for path in args.compare)
        a, b = first.pop("trace"), second.pop("trace")
        mismatch = next(((i, left, right) for i, (left, right) in enumerate(zip(a, b)) if left != right), None)
        comparison = {"identical_action_position_trace": a == b, "trace_lengths": [len(a), len(b)],
                      "first_mismatch": mismatch, "runs": [first, second]}
        payload = comparison
    else:
        jobs = []
        if args.replay:
            jobs.append((args.replay, args.bot1_side, {}))
        for run_dir in args.run_dirs:
            with (run_dir / "results.csv").open(newline="", encoding="utf-8") as handle:
                for result in csv.DictReader(handle):
                    if result.get("outcome") not in ("win", "loss", "draw"):
                        continue
                    jobs.append((run_dir / result["replay"], result["bot1_side"],
                                 {"run_dir": str(run_dir.resolve()), "game": result["game"],
                                  "map": Path(result["map"]).stem, "outcome": result["outcome"]}))
        if not jobs:
            parser.error("provide run directories, --replay, or --compare")
        analyses = []
        for path, side, metadata in jobs:
            analysis = analyze_replay(path, side)
            analysis.update(metadata)
            analyses.append(analysis)
        payload = {"metric_definition": __doc__, "games": analyses}
    if args.output_prefix:
        args.output_prefix.parent.mkdir(parents=True, exist_ok=True)
        json_path = args.output_prefix.with_suffix(".json")
        json_path.write_text(json.dumps(payload, indent=2) + chr(10), encoding="utf-8")
        analyses = payload.get("games", payload.get("runs", []))
        rows = []
        for game in analyses:
            metadata = {key: game.get(key, "") for key in ("run_dir", "game", "map", "replay", "winner", "outcome", "bot1_side")}
            for row in game["checkpoints"] + game["final"]:
                rows.append({**metadata, **row})
        csv_path = args.output_prefix.with_suffix(".csv")
        with csv_path.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=list(rows[0]) if rows else [])
            writer.writeheader()
            writer.writerows(rows)
        print(json.dumps({"json": str(json_path), "csv": str(csv_path), "games": len(analyses),
                          **({"identical_action_position_trace": payload["identical_action_position_trace"],
                              "first_mismatch": payload["first_mismatch"]} if args.compare else {})}))
    else:
        print(json.dumps(payload, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
