#!/usr/bin/env python3
"""Summarize fixed dual-opponent suites; distinguish no-action deaths from faults.

Reuses the previous verified log parser and packed replay reader. Raw games
are never excluded based on result or in-game death reason.
"""
import argparse
import csv
import hashlib
import importlib.util
import json
import statistics
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OPPONENTS = ('V3_portal', 'xxqztest')
MAPS = ('autarky','default','devil','portals','dilemma','queen_of_spades',
        'schooltime','slithery_fight','trauma','trophy')

def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

old = load_module('prior_audit', ROOT/'experiment_results/protect_vs_v3_20260928/verify_final.py')
reader = load_module('replay_reader', ROOT/'experiment_results/protect_local_20260927/analyze_replays.py')

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def key(row):
    return row['opponent'], int(row['seed']), row['map'], row['bot1_side']

def counts(rows):
    c = Counter(r['outcome'] for r in rows)
    completed = sum(c[x] for x in ('win','loss','draw'))
    return dict(wins=c['win'], losses=c['loss'], draws=c['draw'],
                failures=len(rows)-completed, completed=completed,
                win_rate=c['win']/completed if completed else None)

def inspect(directory, seeds, verify_replays):
    directory = Path(directory).resolve()
    config = json.loads((directory/'run.json').read_text())
    rows = list(csv.DictReader((directory/'results.csv').open()))
    expected = {(o,s,m,a) for o in OPPONENTS for s in seeds for m in MAPS for a in 'AB'}
    frequency = Counter(key(r) for r in rows)
    checks = dict(exact_conditions=set(frequency)==expected and all(n==1 for n in frequency.values()),
                  expected_count=len(rows)==len(expected),
                  all_completed=all(r['outcome'] in ('win','loss','draw') and not r['error'] for r in rows),
                  configured_seeds=config['seeds']==seeds,
                  configured_opponents=[Path(p).name for p in config['sources'][1:]]==list(OPPONENTS),
                  maps_current=all(Path(p).is_file() and digest(p)==h for p,h in config['maps'].items()),
                  all_ten_maps={Path(p).stem for p in config['maps']}==set(MAPS))
    manifests = {}
    for i, source in enumerate(config['sources']):
        source = Path(source)
        snapshot = directory/'sources'/str(i)/source.name
        manifest = old.production(snapshot)
        manifests[source.name] = manifest
        checks['source_matches_'+source.name] = bool(manifest) and manifest==old.production(source)
        binary = Path(config['built'][i])
        checks['binary_exists_'+source.name] = binary.is_file() and binary.stat().st_size>0
    games = []
    for row in rows:
        label = str(row['game'])
        replay, log = directory/row['replay'], directory/row['log']
        exists = replay.is_file() and replay.stat().st_size>0 and log.is_file() and log.stat().st_size>0
        checks['artifacts_'+label] = exists
        if not exists:
            continue
        clean, notices = old.log_notices(log.read_text(errors='replace'))
        checks['seed_'+label] = old.log_seed_values(clean)==[int(row['seed'])]
        matches = list(old.RESULT.finditer(clean))
        checks['result_'+label] = len(matches)==1 and (matches[0].group(1) or 'draw')==row['winner'] and int(matches[0].group(2))==int(row['rounds'])
        expected_outcome = 'draw' if row['winner']=='draw' else 'win' if row['winner']==row['bot1_side'] else 'loss'
        checks['outcome_'+label] = row['outcome']==expected_outcome
        checks['runtime_'+label] = not notices['runtime_notices']
        detail = dict(condition=key(row), runtime_notices=notices['runtime_notices'],
                      no_action_events=notices['invalid_action_log_events'])
        if verify_replays:
            root = reader.Message(replay.read_bytes()).structure(0,0)
            result = root.child(4)
            winner = 'AB'[result.number(6,'H')] if result.number(4,'H') else 'draw'
            checks['replay_winner_'+label] = winner==row['winner']
            expected_hash = next((h for p,h in config['maps'].items() if Path(p).stem==row['map']),None)
            checks['replay_map_'+label] = digest(replay) is not None and hashlib.sha256(root.text(0).encode()).hexdigest()==expected_hash
            standings = {side:dict(population=result.child(i).number(0),max_length=result.child(i).number(4),total_length=result.child(i).number(8)) for i,side in enumerate('AB')}
            detail.update(replay_sha256=digest(replay),standings=standings)
        games.append(detail)
    by_opponent = {}
    for opponent in OPPONENTS:
        group = [r for r in rows if r['opponent']==opponent]
        by_seed = {str(s):counts([r for r in group if int(r['seed'])==s]) for s in seeds}
        values = [x['win_rate'] for x in by_seed.values() if x['win_rate'] is not None]
        by_opponent[opponent] = dict(**counts(group),by_seed=by_seed,
            mean_seed_win_rate=statistics.mean(values) if values else None,
            sample_sd=statistics.stdev(values) if len(values)>1 else None,
            by_map={m:counts([r for r in group if r['map']==m]) for m in MAPS})
    return dict(directory=str(directory),bot=Path(config['sources'][0]).name,
                complete_and_valid=all(checks.values()),checks=checks,
                missing=sorted(expected-set(frequency)),unexpected=sorted(set(frequency)-expected),
                by_opponent=by_opponent,maps=config['maps'],source_manifests=manifests,
                rows=rows,games=games,verified_replays=verify_replays)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('runs',nargs='+')
    ap.add_argument('--seeds',nargs='+',type=int,required=True)
    ap.add_argument('--baseline',required=True)
    ap.add_argument('--output',required=True)
    ap.add_argument('--verify-replays',action='store_true')
    args=ap.parse_args()
    baseline=inspect(args.baseline,args.seeds,args.verify_replays)
    runs=[]
    for path in args.runs:
        run=baseline if Path(path).resolve()==Path(args.baseline).resolve() else inspect(path,args.seeds,args.verify_replays)
        paired={}
        base_rows={key(r):r for r in baseline['rows']}
        for opponent in OPPONENTS:
            transitions=Counter((base_rows[key(r)]['outcome'],r['outcome']) for r in run['rows'] if r['opponent']==opponent and key(r) in base_rows)
            paired[opponent]=dict(transitions={a+'->'+b:n for (a,b),n in sorted(transitions.items())},pairs=sum(transitions.values()),
                loss_to_win=transitions['loss','win'],win_to_loss=transitions['win','loss'])
        run['paired_to_baseline']=paired
        run['opponents_and_maps_match_baseline']=run['maps']==baseline['maps'] and all(run['source_manifests'].get(o)==baseline['source_manifests'].get(o) for o in OPPONENTS)
        run['complete_and_valid'] &= run['opponents_and_maps_match_baseline'] and baseline['complete_and_valid']
        runs.append(run)
    payload=dict(baseline=baseline['directory'],seeds=args.seeds,metric='wins/completed, separately for each opponent; no significance test',
                 no_action_policy='In-game death event; does not itself establish runtime failure or intentional sacrifice.',runs=runs)
    Path(args.output).write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps([dict(bot=r['bot'],valid=r['complete_and_valid'],failed_checks=[k for k,v in r['checks'].items() if not v],
                         score={o:(r['by_opponent'][o]['wins'],r['by_opponent'][o]['completed']) for o in OPPONENTS}) for r in runs],ensure_ascii=False))
    return 0 if all(r['complete_and_valid'] for r in runs) else 1

if __name__=='__main__':
    raise SystemExit(main())
