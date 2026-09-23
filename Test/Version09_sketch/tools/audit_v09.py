#!/usr/bin/env python3
"""Audit V09 bounds/parity and descriptive survival, trade, mobility and economy metrics."""
from collections import Counter
import gzip
import json
from pathlib import Path
import sys
from benchmark_v05 import parse_run


def read_trace(out, match):
    with gzip.open(out/match['log'],'rt') as f:return parse_run(f.read())


def metrics(obs, side):
    deaths=[d for d in obs['deaths'] if d['team']==side]
    turns=obs['turns'][side]
    pairs=[]
    # Consecutive H callbacks in one round are the engine's victim/attacker
    # death sequence. Lengths are last inputs, not exact collision lengths.
    i=0
    while i+1<len(obs['deaths']):
        a,b=obs['deaths'][i:i+2]
        if a['reason']==b['reason']=='H' and a['round']==b['round']:
            if a['team']!=b['team'] and a['last_observed_length'] is not None and b['last_observed_length'] is not None:
                own,enemy=(a,b) if a['team']==side else (b,a)
                pairs.append(enemy['last_observed_length']-own['last_observed_length'])
            i+=2
        else:i+=1
    return {'turns':turns,'deaths':len(deaths),'death_causes':dict(Counter(d['reason'] for d in deaths)),
            'deaths_per_1000_turns':1000*len(deaths)/turns,
            'observed_head_trade_length_differences':pairs,
            'surviving_growth_lower_bound':obs['surviving_growth_lower_bound'].get(side,0),
            'growth_lower_bound_per_1000_turns':1000*obs['surviving_growth_lower_bound'].get(side,0)/turns}


def main():
    out=Path(sys.argv[1]).resolve();summary=json.loads((out/'summary.json').read_text())
    assert summary['all_passed'] and len(summary['matches'])==28
    assert sum(m.get('parity_passed',False) for m in summary['matches'])==16
    events=Counter();examples={};turns=0
    for m in summary['matches']:
        if m['mode']!='profile':continue
        p=read_trace(out,m)
        commands={(t['round'],t['id']):t['commands'] for t in p['turns']}
        for row in p['stats']:
            c=row['counters'];turns+=1
            assert c['tiles_scanned']==c['perception_tiles_scanned']==49
            assert c['route_calls']<=1 and c['route_nodes']<=320
            assert c['micro_candidates']<=4 and c['micro_nodes']<=196 and c['micro_selected']<=1
            assert c['exceptions']==c['emergency_defaults']==0
            for k,v in c.items():
                if k.startswith('micro_'):
                    events[k]+=v
                    if v and k not in examples:
                        examples[k]={'map':m['map'],'side':m['candidate_side'],'round':row['round'],'id':row['id'],
                                     'commands':commands[row['round'],row['id']]}
    comparisons=[];wins=0
    for m in summary['matches']:
        if m['mode']!='release':continue
        side=m['candidate_side'];wins+=m['outcome']['winner']==side
        ref=next(r for r in summary['matches'] if r['map']==m['map'] and r['mode']=='reference')
        a=json.loads((out/ref['observations']).read_text());b=json.loads((out/m['observations']).read_text())
        old={(t['round'],t['id'],t['team']):t for t in read_trace(out,ref)['turns']}
        first=None
        for t in read_trace(out,m)['turns']:
            previous=old.get((t['round'],t['id'],t['team']))
            action=lambda x:[c for c in x['commands'] if c.startswith(('MOVE ','SPLIT '))]
            if previous and action(previous)!=action(t):
                first={'round':t['round'],'id':t['id'],'team':t['team'],'v08':previous['commands'],'v09':t['commands']};break
        checkpoints=[]
        for r in [25,50,80,100]:
            aa=next((x for x in a['rounds'] if x['round']==r and x['team']==side),None)
            bb=next((x for x in b['rounds'] if x['round']==r and x['team']==side),None)
            if aa and bb:checkpoints.append({'round':r,'v08':aa,'v09':bb})
        comparisons.append({'map':m['map'],'side':side,'outcome':m['outcome'],'v08':metrics(a,side),'v09':metrics(b,side),
                            'first_action_change':first,'checkpoints':checkpoints})
    totals={}
    for version in ['v08','v09']:
        rows=[c[version] for c in comparisons];n=sum(r['turns'] for r in rows);d=sum(r['deaths'] for r in rows);g=sum(r['surviving_growth_lower_bound'] for r in rows)
        trades=[x for r in rows for x in r['observed_head_trade_length_differences']]
        totals[version]={'turns':n,'deaths':d,'deaths_per_1000_turns':1000*d/n,'surviving_growth_lower_bound':g,
                         'growth_lower_bound_per_1000_turns':1000*g/n,'head_trade_pairs':len(trades),
                         'mean_last_observed_enemy_minus_own_length':sum(trades)/len(trades) if trades else None}
    selected=events['micro_selected']
    report={'all_technical_checks_passed':True,'profile_turns':turns,'ablation_parity_matches':8,'profile_parity_matches':8,
            'wins':wins,'non_wins':8-wins,'micro_events':dict(events),'paired_metrics':totals,
            'selected_mean_predicted_space':events['micro_space']/selected,
            'selected_mean_known_exits':events['micro_exits']/selected,
            'selected_trap_fraction':events['micro_trap']/selected,
            'examples':examples,'comparisons':comparisons,
            'limitations':['Four fixed maps with swapped sides; not an estimated general win rate.',
                           'Paired metrics use V08 self-play as reference; opponents respond differently after divergence.',
                           'Normal movement predictions omit future opponent moves, sprints, portals, splitting and new pearl spawns.',
                           'Trap and space telemetry are predictions for selected normal moves, not measured post-turn safety or a V08 trap-rate comparison.',
                           'Positive surviving length deltas undercount pearl growth after sprints/splits and deaths.',
                           'Head-trade lengths are latest pre-action inputs; paired H events are descriptive and include legacy attacks.',
                           'Native runner validation does not establish contest computation-budget compliance.']}
    (out/'candidate-audit.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k not in ['examples','comparisons','limitations']},indent=2))


if __name__=='__main__':main()
