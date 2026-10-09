#!/usr/bin/env python3
"""Summarize EXP_METRIC log lines from one verbose benchmark result folder."""

import csv
import re
import sys
from collections import defaultdict
from pathlib import Path


METRIC = re.compile(
    r"^LOG EXP_METRIC (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)$",
    re.MULTILINE,
)


def main(folder: str) -> int:
    root = Path(folder)
    results_path = root / "results.csv"
    if not results_path.is_file():
        raise SystemExit(f"missing {results_path}")

    # (bot side, map, checkpoint) -> rows of (unique tiles, coverage %, repeat %)
    buckets = defaultdict(list)
    with results_path.open(newline="", encoding="utf-8") as handle:
        for result in csv.DictReader(handle):
            if result["outcome"] not in ("win", "loss", "draw"):
                continue
            replay_log = root / result["log"]
            bot_side = result["bot1_side"]
            map_name = Path(result["map"]).stem
            for match in METRIC.finditer(replay_log.read_text(encoding="utf-8", errors="replace")):
                round_num, team, _dragon, _length, unique, coverage, samples, _repeats, rate = map(int, match.groups())
                if round_num not in (100, 200) or samples < 20:
                    continue
                side = "A" if team == 65 else "B"
                owner = "bot1" if side == bot_side else "bot2"
                buckets[(owner, map_name, round_num)].append((unique, coverage / 10.0, rate / 10.0))

    print("bot,map,round,dragon_samples,mean_unique_tiles,mean_personal_coverage_percent,mean_recent_repeat_percent")
    for (owner, map_name, round_num), values in sorted(buckets.items()):
        n = len(values)
        mean_unique = sum(row[0] for row in values) / n
        mean_coverage = sum(row[1] for row in values) / n
        mean_repeat = sum(row[2] for row in values) / n
        print(f"{owner},{map_name},{round_num},{n},{mean_unique:.1f},{mean_coverage:.1f},{mean_repeat:.1f}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: python3 analyze_exploration_metrics.py REPLAY_RUN_DIR")
    raise SystemExit(main(sys.argv[1]))
