#!/usr/bin/env python3
"""Audit paired V07/V06 profile turns from an existing benchmark archive."""
from collections import Counter
import gzip
import json
from pathlib import Path
import sys
from benchmark_v05 import parse_run


def main():
    out = Path(sys.argv[1]).resolve()
    summary = json.loads((out / 'summary.json').read_text())
    assert summary['all_passed'] and len(summary['matches']) == 28
    changed = {'pocket_calls', 'pocket_nodes', 'pocket_degree_nodes',
               'tactical_enemy_checks', 'escape_calls', 'escape_edges'}
    turns = 0
    more_escapes = 0
    reviews = []
    for row in summary['matches']:
        if row['mode'] != 'profile':
            continue
        baseline = next(m for m in summary['matches'] if m['map'] == row['map']
                        and m['candidate_side'] == row['candidate_side'] and m['mode'] == 'baseline_profile')
        with gzip.open(out / row['log'], 'rt') as stream:
            new = parse_run(stream.read())
        with gzip.open(out / baseline['log'], 'rt') as stream:
            old = parse_run(stream.read())
        assert old['turns'] == new['turns'] and old['outcome'] == new['outcome']
        assert len(old['stats']) == len(new['stats'])
        for before, after in zip(old['stats'], new['stats']):
            assert (before['round'], before['id'], before['team']) == (after['round'], after['id'], after['team'])
            a, b = before['counters'], after['counters']
            for key in a.keys() - changed:
                assert a[key] == b[key], (row['map'], before['round'], key)
            assert b['tiles_scanned'] == b['perception_tiles_scanned'] == 49
            assert b['component_builds'] == 1 and b['component_fallbacks'] == 0
            assert b['component_lookups'] == a['pocket_calls']
            assert b['component_regions'] <= a['pocket_calls']
            assert b['component_nodes'] <= a['pocket_nodes']
            assert b['pocket_calls'] == b['pocket_nodes'] == b['pocket_degree_nodes'] == 0
            assert b['threat_builds'] <= 1
            assert b['tactical_enemy_checks'] == 0
            assert b['threat_enemy_checks'] <= a['tactical_enemy_checks']
            more_escapes += b['escape_calls'] > a['escape_calls']
            turns += 1
        labels = Counter()
        examples = {}
        for turn in new['turns']:
            if turn['team'] != row['candidate_side']:
                continue
            for command in turn['commands']:
                if command.startswith('INDICATOR '):
                    label = command.split()[1]
                    labels[label] += 1
                    examples.setdefault(label, turn)
        reviews.append({'map': row['map'], 'side': row['candidate_side'], 'outcome': row['outcome'],
                        'indicator_counts': dict(labels), 'first_examples': examples})
    a, b = summary['baseline_profile']['counters'], summary['profile']['counters']
    metrics = {}
    for name, old_key, new_keys in [
        ('flood_fills', 'pocket_calls', ['component_regions', 'pocket_calls']),
        ('flood_nodes', 'pocket_nodes', ['component_nodes', 'pocket_nodes']),
        ('enemy_analysis', 'tactical_enemy_checks', ['threat_enemy_checks', 'tactical_enemy_checks']),
        ('escape_calls', 'escape_calls', ['escape_calls'])]:
        old_total = a[old_key]['total']
        new_total = sum(b[key]['total'] for key in new_keys)
        metrics[name] = {'v06_total': old_total, 'v07_total': new_total,
                         'reduction_percent': round((1-new_total/old_total)*100, 3)}
    report = {'all_passed': True, 'compared_profile_turns': turns,
              'identical_counters_per_turn': 24, 'turns_with_more_escape_calls': more_escapes,
              'metrics': metrics, 'review_method': 'Full action and replay-annotation streams, branch counts and examples; no graphical replay-viewer review.',
              'reviews': reviews}
    (out / 'per-turn-audit.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: v for k, v in report.items() if k != 'reviews'}, indent=2))


if __name__ == '__main__':
    main()
