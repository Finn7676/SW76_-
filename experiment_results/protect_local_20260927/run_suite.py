#!/usr/bin/env python3
"""Fixed-seed, side-balanced official-map matches. Failures are not losses."""
import argparse, concurrent.futures, csv, hashlib, json, subprocess, sys, time
from collections import Counter
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT))
import benchmark as bench

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--bot',default='tyhh_test/tyhh_qos_protect_route')
    ap.add_argument('--opponents',nargs='+',default=['tyhh','tyhh_test/V3_portal'])
    ap.add_argument('--maps',nargs='+',default=['all'])
    ap.add_argument('--seeds',nargs='+',type=int,default=[20260927,20260928,20260929])
    ap.add_argument('--jobs',type=int,default=2)
    ap.add_argument('--output',required=True)
    ap.add_argument('--verbose',action='store_true')
    a=ap.parse_args(); out=Path(a.output).resolve()
    out.mkdir(parents=True,exist_ok=False)
    maps=bench.resolve_maps(a.maps)
    sources=[bench.resolve_bot(a.bot)]+[bench.resolve_bot(s) for s in a.opponents]
    built=[bench.prepare_bot(s,out/'sources'/str(i)/s.name) for i,s in enumerate(sources)]
    config={'sources':list(map(str,sources)),'built':list(map(str,built)),
            'hashes':{str(s/f):sha(s/f) for s in sources for f in ['main.cpp','helper.hpp','bot.toml'] if (s/f).is_file()},
            'maps':{str(m):sha(m) for m in maps},'seeds':a.seeds,'jobs':a.jobs,'verbose':a.verbose}
    (out/'run.json').write_text(json.dumps(config,indent=2)+'\n')
    work=[]
    for oi in range(1,len(sources)):
        for seed in a.seeds:
            for m in maps:
                for side in ['A','B']: work.append((len(work)+1,oi,seed,m,side))
    fields=['game','opponent','seed','map','bot1_side','outcome','winner','rounds','reason','seconds','replay','log','error']
    def run(task):
        i,oi,seed,m,side=task
        stem=f'{i:04d}_{sources[oi].name}_{m.stem}_seed{seed}_{side}'
        replay=out/(stem+'.replay');log=out/(stem+'.log')
        pair=[built[0],built[oi]] if side=='A' else [built[oi],built[0]]
        cmd=['unswbc','run',str(m),*map(str,pair),'--seed',str(seed),'-o',str(replay)]
        if a.verbose: cmd.append('-v')
        start=time.monotonic();winner='';error='';outcome='failed';rounds='';reason=''
        try:
            p=subprocess.run(cmd,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,errors='replace',timeout=240)
            log.write_text(p.stdout)
            if p.returncode: raise ValueError(f'CLI exit {p.returncode}')
            winner=bench.parse_result(p.stdout)
            if not replay.is_file() or replay.stat().st_size==0: raise ValueError('Missing replay')
            import re
            match=re.search(r'(?:team [AB] wins|draw) after (\d+) rounds[^\n]*',bench.ANSI.sub('',p.stdout))
            if match: rounds=match.group(1);reason=match.group(0)
            outcome='draw' if winner=='draw' else 'win' if winner==side else 'loss'
        except Exception as e:
            error=str(e)
            if not log.exists(): log.write_text(error+'\n')
        return dict(zip(fields,[i,sources[oi].name,seed,m.stem,side,outcome,winner,rounds,reason,round(time.monotonic()-start,3),replay.name,log.name,error]))
    rows=[]
    with (out/'results.csv').open('w',newline='') as f:
        writer=csv.DictWriter(f,fieldnames=fields);writer.writeheader();f.flush()
        with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
            futures=[pool.submit(run,t) for t in work]
            for future in concurrent.futures.as_completed(futures):
                row=future.result();rows.append(row);writer.writerow(row);f.flush()
                print(f'[{len(rows)}/{len(work)}] {row["opponent"]} {row["map"]} seed={row["seed"]} {row["bot1_side"]}: {row["outcome"]} r{row["rounds"]} {row["error"]}',flush=True)
    rows.sort(key=lambda x:x['game'])
    summary={s.name:dict(Counter(r['outcome'] for r in rows if r['opponent']==s.name)) for s in sources[1:]}
    (out/'summary.json').write_text(json.dumps({'planned':len(work),'completed':len(rows),'by_opponent':summary},indent=2)+'\n')
    print(json.dumps(summary,indent=2),flush=True)
    return int(any(r['outcome']=='failed' for r in rows))
if __name__=='__main__': raise SystemExit(main())
