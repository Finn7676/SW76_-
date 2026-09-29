from unswbc.engine import EngineModule
import json,pathlib

board='''MAP 11 11
MAP_NAME portal_roundtrip
TILE_COUNT 0
EDGE_COUNT 2
EDGE 135 2 0
EDGE 140 2 0
DRAGON_COUNT 2
DRAGON 0 2 2 5 1 5
DRAGON 1 2 5 9 4 9
END
'''
class Done(Exception):pass
for label,actions in [('immediate_reverse',['E','W']),('return_from_other_side',['E','N','W','S','E','N'])]:
 rows=[];deaths=[]
 def reply(i,block):
  lines=block.decode().splitlines();r=int(lines[0].split()[1])
  if r>=len(actions):raise Done()
  if i==0:
   k=next(n for n,l in enumerate(lines) if l.startswith('DRAGON_BODIES'))
   parts=[l.split() for l in lines[k+1:k+1+int(lines[k].split()[1])]]
   own=[p for p in parts if p[0]=='A' and p[1]=='0']
   rows.append({'round':r,'parts':own,'move':actions[r]})
   return ('MOVE '+actions[r]+'\n').encode()
  return b'MOVE E\n'
 def death(i,r,why):deaths.append({'id':i,'round':r,'reason':why})
 engine=EngineModule()
 try:engine.run(board.encode(),reply,death,seed=7,on_notice=lambda s:print('NOTICE',s))
 except Done:pass
 result={'case':label,'turns':rows,'deaths':deaths}
 print(json.dumps(result,ensure_ascii=False))

 if label=='immediate_reverse':
  assert any(d['id']==0 and d['round']==1 and d['reason']=='S' for d in deaths)
 else:
  assert not any(d['id']==0 for d in deaths)
  assert rows[5]['parts'][0][2:4]==['3','5']
