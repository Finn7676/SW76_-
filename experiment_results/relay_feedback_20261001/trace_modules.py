#!/usr/bin/env python3
"""Attribute intervention logs to candidate/opponent using replay team lineage.

Logs establish policy activation, not successful pearl recovery or causation.
The samples retain the original round/dragon/message for replay inspection.
"""
import argparse
import csv
import importlib.util
import json
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    'trace_reader', ROOT/'experiment_results/protect_local_20260927/analyze_replays.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def analyze(path, candidate_side, prefixes, sample_limit):
    root = reader.Message(path.read_bytes()).structure(0, 0)
    _, _, _, teams = reader.parse_map(root.text(0))
    round_number = -1
    counts = {side: Counter() for side in 'AB'}
    samples = {side: defaultdict(list) for side in 'AB'}
    for event in root.structs(3):
        kind, data = event.number(0, 'H'), event.child(0)
        if kind == 0:
            round_number = data.number(0)
        elif kind == 10:
            child, side = data.number(4), 'AB'[data.number(8, 'H')]
            teams[child] = side
        elif kind == 6:
            ident, message = data.number(0), data.text(0)
            words = message.split()
            if words and words[0] == 'LOG':
                words = words[1:]
            if not words or not any(words[0].startswith(p) for p in prefixes):
                continue
            side = teams[ident]  # Unknown lineage is a parse error, not ignored.
            tag = words[0]
            counts[side][tag] += 1
            if len(samples[side][tag]) < sample_limit:
                samples[side][tag].append(dict(round=round_number, dragon=ident, message=message))
    return dict(replay=str(path.resolve()), candidate_side=candidate_side,
                counts={('candidate' if s == candidate_side else 'opponent'): dict(c)
                        for s, c in counts.items()},
                samples={('candidate' if s == candidate_side else 'opponent'): dict(c)
                         for s, c in samples.items()})


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('runs', nargs='+', type=Path)
    ap.add_argument('--prefixes', nargs='+', required=True)
    ap.add_argument('--sample-limit', type=int, default=3)
    ap.add_argument('--output', required=True, type=Path)
    args = ap.parse_args()
    games, totals = [], defaultdict(Counter)
    for directory in args.runs:
        rows = list(csv.DictReader((directory/'results.csv').open()))
        for row in sorted(rows, key=lambda r: int(r['game'])):
            if row['outcome'] not in ('win', 'loss', 'draw'):
                raise ValueError(f"Incomplete match: {directory}/{row['game']}")
            result = analyze(directory/row['replay'], row['bot1_side'], args.prefixes, args.sample_limit)
            result.update(run=directory.name, opponent=row['opponent'], map=row['map'],
                          seed=int(row['seed']), outcome=row['outcome'])
            games.append(result)
            for owner, counts in result['counts'].items():
                totals[directory.name, row['opponent'], owner].update(counts)
        print(f"scanned {directory.name}: {len(rows)} games", flush=True)
    payload = dict(definition=__doc__, games=games,
                   totals=[dict(run=k[0], opponent=k[1], owner=k[2], tags=dict(v))
                           for k, v in sorted(totals.items())])
    args.output.write_text(json.dumps(payload, ensure_ascii=False, indent=2)+'\n')
    print(json.dumps(payload['totals'], ensure_ascii=False))


if __name__ == '__main__':
    main()
