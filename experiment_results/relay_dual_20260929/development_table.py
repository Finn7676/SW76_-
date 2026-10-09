#!/usr/bin/env python3
"""Print complete fixed-development suites only; never inspect final matches."""
import csv
import json
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parent
OPPONENTS = ('V3_portal', 'xxqztest')
SEEDS = (73001, 73002)

def main():
    groups = defaultdict(list)
    for path in sorted(ROOT.glob('*_dev*')):
        if not (path / 'summary.json').is_file():
            continue
        run = json.loads((path / 'run.json').read_text())
        if any(seed not in SEEDS for seed in run['seeds']):
            continue
        rows = list(csv.DictReader((path / 'results.csv').open()))
        expected = {(o, str(s), Path(m).stem, side) for o in OPPONENTS
                    for s in run['seeds'] for m in run['maps'] for side in 'AB'}
        conditions = [(r['opponent'], r['seed'], r['map'], r['bot1_side']) for r in rows]
        if set(conditions) != expected or len(conditions) != len(expected):
            raise ValueError(f'incomplete/duplicate conditions: {path}')
        if any(r['error'] or r['outcome'] not in ('win', 'loss', 'draw') for r in rows):
            raise ValueError(f'failed games: {path}')
        groups[Path(run['sources'][0]).name].extend(rows)
    print('| Version | Seeds | V3 W/L/D | xxqztest W/L/D |')
    print('|---|---|---|---|')
    for bot, rows in sorted(groups.items()):
        keys = [(r['opponent'], r['seed'], r['map'], r['bot1_side']) for r in rows]
        if len(set(keys)) != len(keys):
            raise ValueError(f'duplicate development games for {bot}')
        scores = []
        for opponent in OPPONENTS:
            group = [r for r in rows if r['opponent'] == opponent]
            scores.append('/'.join(str(sum(r['outcome'] == outcome for r in group))
                                   for outcome in ('win', 'loss', 'draw')))
        seeds = ','.join(sorted({r['seed'] for r in rows}))
        print(f'| {bot} | {seeds} | {scores[0]} | {scores[1]} |')

if __name__ == '__main__':
    main()
