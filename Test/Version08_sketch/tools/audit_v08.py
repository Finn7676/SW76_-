#!/usr/bin/env python3
"""Summarize V08 technical checks and paired-side exploration outcomes."""
import gzip
import json
from pathlib import Path
import sys
from benchmark_v05 import parse_run


def main():
    out=Path(sys.argv[1]).resolve()
    s=json.loads((out/'summary.json').read_text())
    assert s['all_passed'] and len(s['matches'])==28
    assert sum(m.get('parity_passed',False) for m in s['matches'])==16
    profiles=[m for m in s['matches'] if m['mode']=='profile']
    count=0;events={};examples={}
    for m in profiles:
        with gzip.open(out/m['log'],'rt') as stream:parsed=parse_run(stream.read())
        for row in parsed['stats']:
            c=row['counters'];count+=1
            assert c['tiles_scanned']==c['perception_tiles_scanned']==49
            assert c['route_calls']<=1 and c['route_nodes']<=320
            assert c['exceptions']==c['emergency_defaults']==0
            for k,v in c.items():
                if k.startswith('nav_'):
                    events[k]=events.get(k,0)+v
                    if v and k not in examples:
                        examples[k]={'map':m['map'],'round':row['round'],'id':row['id'],'team':row['team'],
                                     'commands':next(t['commands'] for t in parsed['turns'] if t['round']==row['round'] and t['id']==row['id'] and t['team']==row['team'])}
    comparisons=[];wins=0;losses=0
    for m in s['matches']:
        if m['mode']!='release':continue
        side=m['candidate_side'];wins+=m['outcome']['winner']==side;losses+=m['outcome']['winner'] not in [side,'draw']
        ref=next(r for r in s['matches'] if r['map']==m['map'] and r['mode']=='reference')
        a=json.loads((out/ref['observations']).read_text());b=json.loads((out/m['observations']).read_text())
        points=[]
        for r in [10,25,50,80,100]:
            old=next((x for x in a['rounds'] if x['team']==side and x['round']==r),None)
            new=next((x for x in b['rounds'] if x['team']==side and x['round']==r),None)
            if old and new:points.append({'round':r,'v07':old,'v08':new})
        comparisons.append({'map':m['map'],'side':side,'outcome':m['outcome'],'checkpoints':points,
                            'v07_loops':a['oscillating_turns'].get(side,0),'v08_loops':b['oscillating_turns'].get(side,0),
                            'v07_turns':a['turns'][side],'v08_turns':b['turns'][side],
                            'first_behavior_change':m['first_behavior_change']})
    aggregate={}
    for r in [10,25,50,80,100]:
        pairs=[p for c in comparisons for p in c['checkpoints'] if p['round']==r]
        aggregate[str(r)]={'paired_sides':len(pairs)}
        for key in ['coverage_fraction','vision_overlap','nearest_ally_distance','observed_team_length']:
            valid=[p for p in pairs if p['v07'][key] is not None and p['v08'][key] is not None]
            aggregate[str(r)][key]={version:round(sum(p[version][key] for p in valid)/len(valid),5) if valid else None for version in ['v07','v08']}
    loops={version:sum(c[version+'_loops'] for c in comparisons) for version in ['v07','v08']}
    loop_rates={version:round(1000*loops[version]/sum(c[version+'_turns'] for c in comparisons),4) for version in ['v07','v08']}
    report={'all_technical_checks_passed':True,'profile_turns':count,'ablation_parity_matches':8,'profile_parity_matches':8,
            'wins':wins,'losses':losses,'draws':8-wins-losses,'averaged_checkpoints':aggregate,
            'observed_loops':loops,'loops_per_thousand_turns':loop_rates,'navigation_events':events,
            'recovery_examples':examples,'comparisons':comparisons,
            'limitations':['Four fixed maps, one run per side; no statistical win-rate claim.',
                           'Metrics compare against same-side V07 self-play, so opponent responses also change.',
                           'Observed lengths/spacing are latest pre-action values, not simultaneous snapshots or pearl totals.',
                           'Recovery examples and unit tests establish the mechanism; controlled trapped-state recovery-rate comparison is not established.',
                           'Contest CPU/points budgets and graphical replay review not performed.']}
    (out/'navigation-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['recovery_examples','comparisons','limitations']},indent=2))


if __name__=='__main__':main()
