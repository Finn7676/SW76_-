#!/usr/bin/env python3
"""Fixed 40-game paired comparison, reusing the prior source/log/replay audit."""
import argparse, importlib.util, json
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    'fixed_audit', ROOT/'experiment_results/relay_dual_20260929/evaluate.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)

def paired(run, parent):
    previous = {audit.key(r):r for r in parent['rows']}
    result = {}
    for opponent in audit.OPPONENTS:
        changes = Counter((previous[audit.key(r)]['outcome'],r['outcome'])
                          for r in run['rows'] if r['opponent']==opponent)
        result[opponent] = {
            'win_delta':run['by_opponent'][opponent]['wins']-parent['by_opponent'][opponent]['wins'],
            'loss_to_win':changes['loss','win'],
            'win_to_loss':changes['win','loss'],
            'transitions':{a+'->'+b:n for (a,b),n in sorted(changes.items())},
            'by_side':{side:audit.counts([r for r in run['rows']
                        if r['opponent']==opponent and r['bot1_side']==side]) for side in 'AB'},
            'map_win_deltas':{m:run['by_opponent'][opponent]['by_map'][m]['wins']-
                               parent['by_opponent'][opponent]['by_map'][m]['wins']
                              for m in audit.MAPS},
        }
    return result

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('runs',nargs='+')
    ap.add_argument('--baseline',required=True)
    ap.add_argument('--output',required=True)
    ap.add_argument('--verify-replays',action='store_true')
    args = ap.parse_args()
    baseline = audit.inspect(args.baseline,[76001],args.verify_replays)
    runs = []
    for path in args.runs:
        run = baseline if Path(path).resolve()==Path(args.baseline).resolve() else audit.inspect(path,[76001],args.verify_replays)
        run['opponents_and_maps_match_baseline'] = run['maps']==baseline['maps'] and all(
            run['source_manifests'].get(o)==baseline['source_manifests'].get(o) for o in audit.OPPONENTS)
        run['complete_and_valid'] &= baseline['complete_and_valid'] and run['opponents_and_maps_match_baseline']
        run['paired_to_baseline'] = paired(run,baseline)
        deltas = [run['paired_to_baseline'][o]['win_delta'] for o in audit.OPPONENTS]
        run['beneficial_by_prespecified_rule'] = all(d>=0 for d in deltas) and any(d>0 for d in deltas)
        if args.verify_replays:
            for opponent in audit.OPPONENTS:
                group = [g for g in run['games'] if g['condition'][0]==opponent]
                run['by_opponent'][opponent]['terminal_means'] = {
                    owner:{field:sum(g['standings'][g['condition'][3] if owner=='candidate'
                                     else ('B' if g['condition'][3]=='A' else 'A')][field] for g in group)/len(group)
                           for field in ('population','max_length','total_length')}
                    for owner in ('candidate','opponent')}
        runs.append(run)
    payload = {'seed':76001,'games_per_version':40,'baseline':baseline['bot'],
               'metric':'wins separately by opponent; all maps and both sides, no significance claim',
               'runs':runs}
    Path(args.output).write_text(json.dumps(payload,ensure_ascii=False,indent=2)+'\n')
    print(json.dumps([{'bot':r['bot'],'valid':r['complete_and_valid'],
        'scores':{o:[r['by_opponent'][o][k] for k in ('wins','losses','draws')] for o in audit.OPPONENTS},
        'beneficial':r['beneficial_by_prespecified_rule'],
        'deltas':{o:r['paired_to_baseline'][o]['win_delta'] for o in audit.OPPONENTS},
        'failed_checks':[k for k,v in r['checks'].items() if not v]} for r in runs],ensure_ascii=False))
    return 0 if all(r['complete_and_valid'] for r in runs) else 1

if __name__=='__main__': raise SystemExit(main())
