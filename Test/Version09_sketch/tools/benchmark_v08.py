#!/usr/bin/env python3
"""V08 behavior-change benchmark: disabled parity, profile parity and observed map metrics."""
import argparse
import gzip
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from benchmark_v05 import snapshot, execute, parse_run, fingerprint, first_difference, validate_profile, aggregate_stats


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',type=Path,default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--baseline',type=Path,required=True)
    parser.add_argument('--maps',type=Path,nargs='+',required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--pilot',action='store_true')
    parser.add_argument('--extra-flag',action='append',default=[])
    args=parser.parse_args()
    out=args.output.resolve();out.mkdir(parents=True,exist_ok=False)
    manifest={'phase':'V08','candidate_sha256':snapshot(args.candidate,out/'inputs/candidate'),
              'baseline_sha256':snapshot(args.baseline,out/'inputs/baseline'),
              'compiler_version':subprocess.check_output(['clang++','--version'],text=True),
              'maps':[],'pilot':args.pilot,'extra_flags':args.extra_flag}
    for p in args.maps:
        dest=out/'inputs/maps'/p.name;dest.parent.mkdir(exist_ok=True);shutil.copy2(p,dest)
        manifest['maps'].append({'name':p.name,'sha256':hashlib.sha256(dest.read_bytes()).hexdigest()})
    (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    bins=out/'bin';bins.mkdir()
    builds=[('baseline','baseline',[]),('release','candidate',[])]
    if not args.pilot:builds += [('ablation','candidate',['-DXXQZ_V08=0']),('profile','candidate',['-DXXQZ_ENABLE_STATS=1'])]
    for name,folder,flags in builds:
        subprocess.run(['clang++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror',*flags,
                        *(args.extra_flag if folder=='candidate' else []), str(out/'inputs'/folder/'main.cpp'),'-o',str(bins/name)],check=True)
    fields=re.findall(r'StatField\{"([a-z_]+)"',(out/'inputs/candidate/debug.hpp').read_text())
    summary={'phase':'V08','all_passed':True,'matches':[],'pilot':args.pilot}
    wrapper=Path(__file__).with_name('observed_run.py')
    for mapping in manifest['maps']:
        reference=None;release={}
        modes=[('reference',None),('release','A'),('release','B')]
        if not args.pilot:modes += [('ablation','A'),('ablation','B'),('profile','A'),('profile','B')]
        for mode,side in modes:
            name=f'{Path(mapping["name"]).stem}-{mode}-{side or "both"}'
            bots=[bins/'baseline',bins/'baseline']
            if side:bots[side=='B']=bins/mode
            argv=[sys.executable,str(wrapper),str(out/'inputs/maps'/mapping['name']),*map(str,bots),
                  str(out/(name+'.replay')),str(out/(name+'.observations.json'))]
            code,output,elapsed=execute(argv,240)
            with gzip.open(out/(name+'.log.gz'),'wt') as log:log.write(output)
            row={'map':mapping['name'],'mode':mode,'candidate_side':side,'exit_code':code,'wall_seconds':round(elapsed,3),
                 'log':name+'.log.gz','replay':name+'.replay','observations':name+'.observations.json'}
            try:
                if code:raise ValueError(f'runner exit {code}')
                parsed=parse_run(output)
                row.update(outcome=parsed['outcome'],runtime_failures=parsed['failures'],trace_sha256=fingerprint(parsed['turns']))
                if parsed['failures']:raise ValueError('runner failures')
                if mode=='reference':reference=parsed
                elif mode=='release':
                    release[side]=parsed
                    row['first_behavior_change']=first_difference(reference['turns'],parsed['turns'])
                else:
                    expected=reference if mode=='ablation' else release[side]
                    row['parity_passed']=parsed['turns']==expected['turns'] and parsed['outcome']==expected['outcome']
                    if not row['parity_passed']:raise ValueError(f'{mode} parity failed')
                if mode=='profile':
                    validate_profile(parsed,side,fields)
                    row['profile']=aggregate_stats(parsed['stats'])
                    if any(row['profile']['counters'][key]['total'] for key in ['exceptions','emergency_defaults']):
                        raise ValueError('caught exception/default')
                elif parsed['stats']:raise ValueError('unexpected telemetry')
                obs=json.loads((out/row['observations']).read_text())
                row['engine_result']=obs['result']
                if side:
                    row['metrics']={'oscillating_turns':obs['oscillating_turns'].get(side,0),'turns':obs['turns'][side],
                                    'checkpoints':[r for r in obs['rounds'] if r['team']==side and r['round'] in [5,10,25,50,80,100]]}
            except (ValueError,KeyError) as error:
                row['error']=str(error);summary['all_passed']=False
            summary['matches'].append(row)
            (out/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
            print(json.dumps({k:v for k,v in row.items() if k in ['map','mode','candidate_side','outcome','error','parity_passed']}),flush=True)
    return 0 if summary['all_passed'] else 1


if __name__=='__main__':raise SystemExit(main())
