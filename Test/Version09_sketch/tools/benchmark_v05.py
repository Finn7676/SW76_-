#!/usr/bin/env python3
"""Snapshot, build and compare release/profile action traces against a frozen baseline."""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import signal
import subprocess
import sys
import time

ANSI = re.compile(r'\x1b\[[0-?]*[ -/]*[@-~]')
HEADER = re.compile(r'^round (\d+): bot (\d+) \(team ([AB])\) stdout:$')
RESULT = re.compile(r'^(team ([AB]) wins|draw) after (\d+) rounds \(([^)]+)\)')
FAILURE = re.compile(r'^round \d+: bot \d+ \(team [AB]\) .*?(exited\b|ran out of time\b|no valid action\b|points limit|memory limit)')
COMMANDS = {'MOVE', 'SPLIT', 'SONAR', 'DOT', 'LINE', 'INDICATOR', 'LOG', 'ENDTURN'}


def fingerprint(value):
    return hashlib.sha256(json.dumps(value, separators=(',', ':'), sort_keys=True).encode()).hexdigest()


def parse_run(output):
    """Exclude only V05 diagnostic lines; preserve all other per-turn bot output."""
    turns, stats, failures, outcomes = [], [], [], []
    current = None
    for raw in ANSI.sub('', output).splitlines():
        line = raw.strip()
        if match := HEADER.fullmatch(line):
            current = {'round': int(match[1]), 'id': int(match[2]), 'team': match[3], 'commands': []}
            turns.append(current)
        elif line.startswith('LOG V05_STATS '):
            if current is None:
                raise ValueError('Diagnostic line outside a turn')
            values = {}
            for token in line.split()[2:]:
                key, sep, value = token.partition('=')
                if not sep or not value.isdigit() or key in values:
                    raise ValueError(f'Malformed counter: {token}')
                values[key] = int(value)
            if values.pop('round', None) != current['round'] or values.pop('id', None) != current['id']:
                raise ValueError('Counter identity does not match the runner turn')
            if not values:
                raise ValueError('Empty diagnostic record')
            stats.append({'round': current['round'], 'id': current['id'], 'team': current['team'], 'counters': values})
        elif current is not None and line.split(' ', 1)[0] in COMMANDS:
            current['commands'].append(line)
        elif line:
            current = None
        if FAILURE.search(line):
            failures.append(line)
        if match := RESULT.match(line):
            outcomes.append({'winner': match[2] or 'draw', 'rounds': int(match[3]), 'reason': match[4]})
    if len(outcomes) != 1:
        raise ValueError(f'Expected one result, found {len(outcomes)}')
    if not turns or any(not turn['commands'] for turn in turns):
        raise ValueError('Missing verbose turn trace')
    return {'outcome': outcomes[0], 'turns': turns, 'stats': stats, 'failures': failures}


def aggregate_stats(records):
    histograms = defaultdict(Counter)
    for record in records:
        for name, value in record['counters'].items():
            histograms[name][value] += 1
    count = len(records)
    metrics = {}
    for name, histogram in sorted(histograms.items()):
        if sum(histogram.values()) != count:
            raise ValueError(f'Inconsistent counter schema: {name}')
        total = sum(value * n for value, n in histogram.items())
        threshold = (95 * count + 99) // 100
        acc = 0
        p95 = 0
        for value, n in sorted(histogram.items()):
            acc += n
            if acc >= threshold:
                p95 = value
                break
        metrics[name] = {'total': total, 'mean_per_turn': round(total / count, 3), 'p95': p95, 'max': max(histogram)}
    return {'turns': count, 'counters': metrics}


def validate_profile(parsed, side, expected_fields):
    expected = [(t['round'], t['id'], t['team']) for t in parsed['turns'] if t['team'] == side]
    actual = [(t['round'], t['id'], t['team']) for t in parsed['stats']]
    if actual != expected:
        raise ValueError('Expected exactly one diagnostic record per candidate turn, in turn order')
    for row in parsed['stats']:
        if set(row['counters']) != set(expected_fields):
            raise ValueError('Unexpected diagnostic field schema')
        c = row['counters']
        if c['tiles_scanned'] != c['memory_tiles_scanned'] + c['sense_tiles_scanned'] + c['broadcast_tiles_scanned'] + c.get('perception_tiles_scanned', 0):
            raise ValueError('Phase tile counts do not sum to total')
        if c['sonar_accepted'] > c['sonar_received'] or c['sonar_sent'] > 1:
            raise ValueError('Invalid sonar accounting')


def first_difference(reference, candidate):
    for index, (a, b) in enumerate(zip(reference, candidate)):
        if a != b:
            return {'turn_index': index, 'reference': a, 'candidate': b}
    if len(reference) != len(candidate):
        return {'reference_turns': len(reference), 'candidate_turns': len(candidate)}
    return None


def snapshot(source, dest):
    dest.mkdir(parents=True)
    hashes = {}
    for path in sorted(source.iterdir()):
        if path.is_file() and (path.suffix in ('.cpp', '.hpp') or path.name == 'bot.toml'):
            shutil.copy2(path, dest / path.name)
            hashes[path.name] = hashlib.sha256(path.read_bytes()).hexdigest()
    if 'main.cpp' not in hashes:
        raise ValueError(f'Missing main.cpp: {source}')
    return hashes


def execute(argv, timeout):
    started = time.perf_counter()
    process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, start_new_session=True)
    try:
        output, _ = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        os.killpg(process.pid, signal.SIGKILL)
        output, _ = process.communicate()
        return -1, output + '\nBENCHMARK_HOST_TIMEOUT\n', time.perf_counter() - started
    return process.returncode, output, time.perf_counter() - started


def main():
    root = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=root)
    parser.add_argument('--baseline', type=Path, default=root.parent / 'Version04')
    parser.add_argument('--maps', nargs='+', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True, help='New directory; existing results are never overwritten')
    parser.add_argument('--phase', default='V05', help='Roadmap version recorded in the report')
    parser.add_argument('--profile-baseline', action='store_true', help='Also measure a baseline with V05 instrumentation')
    parser.add_argument('--repeats', type=int, default=1)
    parser.add_argument('--timeout', type=float, default=180)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'clang++'))
    args = parser.parse_args()
    if args.repeats < 1 or args.timeout <= 0:
        parser.error('repeats and timeout must be positive')
    candidate, baseline = args.candidate.resolve(), args.baseline.resolve()
    for path in [candidate / 'main.cpp', baseline / 'main.cpp', *args.maps]:
        if not path.is_file():
            parser.error(f'File not found: {path}')
    runner = shutil.which('unswbc')
    if not runner:
        parser.error('unswbc is not installed/on PATH')
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    manifest = {
        'started_at': datetime.now(timezone.utc).isoformat(),
        'candidate': str(candidate), 'baseline': str(baseline),
        'candidate_sha256': snapshot(candidate, out / 'inputs/candidate'),
        'baseline_sha256': snapshot(baseline, out / 'inputs/baseline'),
        'compiler': shlex.split(args.cxx),
        'compiler_version': subprocess.check_output([*shlex.split(args.cxx), '--version'], text=True),
        'runner': runner, 'maps': [],
        'seed': 'No seed option in the installed runner; map snapshots and repeats are recorded.',
    }
    for i, path in enumerate(args.maps):
        copy = out / 'inputs/maps' / f'{i}-{path.name}'
        copy.parent.mkdir(exist_ok=True)
        shutil.copy2(path, copy)
        manifest['maps'].append({'name': path.name, 'path': str(path.resolve()), 'snapshot': str(copy.relative_to(out)),
                                 'sha256': hashlib.sha256(copy.read_bytes()).hexdigest()})
    (out / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    bins = out / 'bin'
    bins.mkdir()
    flags = ['-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror']
    builds = [('baseline', 'baseline', []), ('release', 'candidate', []), ('profile', 'candidate', ['-DXXQZ_ENABLE_STATS=1'])]
    if args.profile_baseline:
        builds.append(('baseline_profile', 'baseline', ['-DXXQZ_ENABLE_STATS=1']))
    for name, folder, extra in builds:
        command = [*shlex.split(args.cxx), *flags, *extra, str(out / 'inputs' / folder / 'main.cpp'), '-o', str(bins / name)]
        subprocess.run(command, check=True)
    fields = re.findall(r'StatField\{"([a-z_]+)"', (out / 'inputs/candidate/debug.hpp').read_text())
    baseline_fields = re.findall(r'StatField\{"([a-z_]+)"', (out / 'inputs/baseline/debug.hpp').read_text()) if args.profile_baseline else []
    summary = {'phase': args.phase, 'all_passed': True, 'matches': [], 'profile': {},
               'scope': 'Behavior preservation and work counts; native wall time is not judge CPU/points usage.'}
    all_stats = []
    baseline_stats = []
    for map_index, mapping in enumerate(manifest['maps']):
        map_path = out / mapping['snapshot']
        for repeat in range(args.repeats):
            reference = None
            variants = [('reference', None), ('release', 'A'), ('release', 'B'), ('profile', 'A'), ('profile', 'B')]
            if args.profile_baseline:
                variants += [('baseline_profile', 'A'), ('baseline_profile', 'B')]
            for mode, side in variants:
                name = f'{map_index}-{Path(mapping["name"]).stem}-r{repeat}-{mode}-{side or "both"}'
                bots = [bins / 'baseline', bins / 'baseline']
                if side:
                    bots[0 if side == 'A' else 1] = bins / mode
                replay = out / (name + '.replay')
                command = [runner, 'run', str(map_path), *map(str, bots), '-v', '-o', str(replay)]
                code, output, elapsed = execute(command, args.timeout)
                with gzip.open(out / (name + '.log.gz'), 'wt') as log:
                    log.write(output)
                record = {'map': mapping['name'], 'repeat': repeat, 'mode': mode, 'candidate_side': side,
                          'exit_code': code, 'wall_seconds': round(elapsed, 3), 'log': name + '.log.gz', 'replay': replay.name}
                try:
                    if code:
                        raise ValueError(f'Runner exit {code}')
                    parsed = parse_run(output)
                    record.update(outcome=parsed['outcome'], runtime_failures=parsed['failures'],
                                  turns=len(parsed['turns']), trace_sha256=fingerprint(parsed['turns']))
                    if mode == 'reference':
                        reference = parsed
                        if parsed['stats']:
                            raise ValueError('Baseline unexpectedly emitted V05 stats')
                    else:
                        if reference is None:
                            raise ValueError('No valid reference run')
                        diff = first_difference(reference['turns'], parsed['turns'])
                        record['matches_reference'] = diff is None and parsed['outcome'] == reference['outcome']
                        if diff:
                            record['first_difference'] = diff
                        if not record['matches_reference']:
                            summary['all_passed'] = False
                    if mode in ('profile', 'baseline_profile'):
                        validate_profile(parsed, side, fields if mode == 'profile' else baseline_fields)
                        record['profile'] = aggregate_stats(parsed['stats'])
                        (all_stats if mode == 'profile' else baseline_stats).extend(parsed['stats'])
                    elif parsed['stats']:
                        raise ValueError('Diagnostics emitted by a non-profile build')
                    if parsed['failures']:
                        summary['all_passed'] = False
                except ValueError as error:
                    record['error'] = str(error)
                    summary['all_passed'] = False
                summary['matches'].append(record)
                summary['profile'] = aggregate_stats(all_stats)
                summary['baseline_profile'] = aggregate_stats(baseline_stats)
                (out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
                print(json.dumps({k: v for k, v in record.items() if k in ('map', 'mode', 'candidate_side', 'outcome', 'matches_reference', 'error')}), flush=True)
    if any(summary[key]['counters'].get(name, {}).get('total', 0)
           for key in ('profile', 'baseline_profile') for name in ('exceptions', 'emergency_defaults')):
        summary['all_passed'] = False
    (out / 'summary.json').write_text(json.dumps(summary, indent=2) + '\n')
    return 0 if summary['all_passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
