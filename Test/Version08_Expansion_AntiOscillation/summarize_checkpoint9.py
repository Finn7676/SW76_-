#!/usr/bin/env python3
import json,re,sys
from pathlib import Path

def main():
    output=Path(sys.argv[1]);rows=[];lines=[];totals={'win':0,'loss':0,'draw':0}
    maps=[line[4:] for line in (output/'config.txt').read_text().splitlines() if line.startswith('map=')]
    if not maps or len(set(maps))!=len(maps):raise ValueError('Missing or duplicate maps')
    for map_name in maps:
        for side in 'AB':
            text=(output/f'{map_name}-candidate-{side}.log').read_text()
            text=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',text)
            if re.search(r'^round \d+: bot .*?(exited\b|ran out of time\b|no valid action\b|points limit|memory limit)',text,re.M):raise ValueError('Runner failure in '+map_name+' '+side)
            results=re.findall(r'^(?:team ([AB]) wins|(draw)) after (\d+) rounds \(([^)]+)\)',text,re.M)
            if len(results)!=1:raise ValueError('Missing/ambiguous result in '+map_name+' '+side)
            winner,draw,rounds,reason=results[0]
            result='draw' if draw else 'win' if winner==side else 'loss'
            bot='draw' if draw else 'Expansion+AntiOscillation' if winner==side else 'V07'
            rows.append(dict(map=map_name,candidate_side=side,winner_team=winner or None,winner_bot=bot,candidate_result=result,end_round=int(rounds),reason=reason))
            totals[result]+=1
            lines.append(f'{map_name} candidate-{side}: winner={bot} (team {winner or "N/A"}); Expansion+AntiOscillation={result}; round={rounds}; reason={reason}')
    lines.append('Expansion+AntiOscillation totals: '+', '.join(f'{k}={v}' for k,v in totals.items()))
    (output/'summary.json').write_text(json.dumps({'games':rows,'totals':totals},indent=2)+'\n')
    summary='\n'.join(lines)+'\n';(output/'summary.txt').write_text(summary);print(summary)
if __name__=='__main__':
    try:main()
    except (ValueError,OSError) as error:raise SystemExit('Summary failed: '+str(error))
