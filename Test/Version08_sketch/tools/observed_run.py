#!/usr/bin/env python3
"""Read-only observation wrapper around the installed UNSW runner callbacks."""
from collections import defaultdict, deque
from dataclasses import asdict
import json
from pathlib import Path
import sys
from unswbc import run
from unswbc.engine import EngineModule, DEBUG_ALL


def decode_turn(dragon_id, block):
    lines = block.decode().splitlines()
    labels = {}
    for i, line in enumerate(lines):
        tokens = line.split()
        if tokens and tokens[0] in ('ROUND', 'LENGTH', 'UNIT_COUNT', 'NUM_MSGS', 'DRAGON_BODIES'):
            labels[tokens[0]] = (i, int(tokens[1]))
    first = labels['NUM_MSGS'][0] + 1 + labels['NUM_MSGS'][1]
    cells = [tuple(map(int, line.split()[:2])) for line in lines[first:first+49]]
    if len(cells) != 49 or len(set(cells)) != 49:
        raise ValueError('Invalid vision block')
    start, count = labels['DRAGON_BODIES']
    heads = [line.split() for line in lines[start+1:start+1+count]
             if int(line.split()[1]) == dragon_id and line.split()[5] == '1']
    if len(heads) != 1:
        raise ValueError('Missing observer head')
    return {'round': labels['ROUND'][1], 'length': labels['LENGTH'][1],
            'head': tuple(map(int, heads[0][2:4])), 'cells': cells}


class Observer:
    def __init__(self):
        self.teams = {}
        self.dimensions = None
        self.latest = {}
        self.coverage = {'A': set(), 'B': set()}
        self.current_cells = {'A': set(), 'B': set()}
        self.samples = defaultdict(int)
        self.history = defaultdict(lambda: deque(maxlen=6))
        self.oscillations = defaultdict(int)
        self.turns = defaultdict(int)
        self.round = None
        self.rounds = []

    def spawn(self, dragon_id, block):
        labels = dict(line.split(maxsplit=1) for line in block.decode().splitlines() if line.strip())
        self.teams[dragon_id] = labels['TEAM']
        dims = tuple(map(int, labels['MAP'].split()))
        if self.dimensions is not None and self.dimensions != dims:
            raise ValueError('Map dimensions changed')
        self.dimensions = dims

    def finish_round(self):
        if self.round is None:
            return
        w, h = self.dimensions
        for team in ['A', 'B']:
            living = [v for k, v in self.latest.items() if self.teams[k] == team]
            nearest = []
            for i, a in enumerate(living):
                distances = []
                for j, b in enumerate(living):
                    if i == j: continue
                    dx, dy = abs(a['head'][0]-b['head'][0]), abs(a['head'][1]-b['head'][1])
                    distances.append(min(dx,w-dx)+min(dy,h-dy))
                if distances: nearest.append(min(distances))
            self.rounds.append({'round': self.round, 'team': team, 'coverage': len(self.coverage[team]),
                                'coverage_fraction': len(self.coverage[team])/(w*h),
                                'vision_overlap': 1-len(self.current_cells[team])/self.samples[team] if self.samples[team] else None,
                                'nearest_ally_distance': sum(nearest)/len(nearest) if nearest else None,
                                'observed_team_length': sum(v['length'] for v in living),
                                'observed_dragons': len(living)})
        self.current_cells = {'A': set(), 'B': set()}
        self.samples.clear()

    def observe(self, dragon_id, block):
        data = decode_turn(dragon_id, block)
        if self.round is not None and self.round != data['round']:
            self.finish_round()
        self.round = data['round']
        team = self.teams[dragon_id]
        self.latest[dragon_id] = data
        self.coverage[team].update(data['cells'])
        self.current_cells[team].update(data['cells'])
        self.samples[team] += len(data['cells'])
        history = self.history[dragon_id]
        if history and data['round'] != history[-1][0]+1:
            history.clear()
        history.append((data['round'], data['head']))
        if (len(history) >= 4 and history[-4][1] == history[-2][1] and history[-3][1] == history[-1][1]) or (len(history) == 6 and history[0][1] == history[4][1] and history[1][1] == history[5][1]):
            self.oscillations[team] += 1
        self.turns[team] += 1


def main():
    map_path, bot_a, bot_b, replay, metrics = sys.argv[1:]
    observer = Observer()
    original = EngineModule.run
    result_data = {}
    def wrapped(self, map_bytes, bot_reply, on_death=None, bot_spawn=None, on_notice=None, debug=DEBUG_ALL):
        def spawn(dragon_id, block):
            observer.spawn(dragon_id, block)
            if bot_spawn: bot_spawn(dragon_id, block)
        def reply(dragon_id, block):
            observer.observe(dragon_id, block)
            return bot_reply(dragon_id, block)
        def death(dragon_id, round_num, reason):
            observer.latest.pop(dragon_id, None)
            if on_death: on_death(dragon_id, round_num, reason)
        result = original(self, map_bytes, reply, death, spawn, on_notice, debug)
        result_data.update(asdict(result))
        return result
    EngineModule.run = wrapped
    try:
        code = run.execute(map_path, [bot_a,bot_b], True, replay, False)
    finally:
        EngineModule.run = original
    observer.finish_round()
    report = {'method': 'Read-only engine input callbacks; live lengths/positions are each dragon latest pre-action observation, not simultaneous end-round snapshots.',
              'dimensions': observer.dimensions, 'result': result_data, 'rounds': observer.rounds,
              'oscillating_turns': dict(observer.oscillations), 'turns': dict(observer.turns)}
    Path(metrics).write_text(json.dumps(report, indent=2)+'\n')
    return code


if __name__ == '__main__':
    raise SystemExit(main())
