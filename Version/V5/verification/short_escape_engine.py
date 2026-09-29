from pathlib import Path
import tempfile,hashlib,shutil,json,os
from unswbc.engine import EngineModule
from unswbc.run import _resolve
from unswbc.sandbox import WasmPool,SandboxBot
source=Path(__file__).resolve().parents[1]
out=Path(os.environ.get('V4_SHORT_ESCAPE_OUT','/tmp/v4-short-escape-results.json'))
class Done(Exception):pass
with tempfile.TemporaryDirectory(prefix='v4-short-escape-') as tmp:
    snapshot=Path(tmp)/'bot';snapshot.mkdir()
    for p in source.iterdir():
        if p.suffix in ('.cpp','.hpp') or p.name=='bot.toml':shutil.copy2(p,snapshot/p.name)
    digest=hashlib.sha256(b''.join(p.read_bytes() for p in sorted(snapshot.iterdir()))).hexdigest()
    with (snapshot/'main.cpp').open('a') as f:f.write('\n// Source fingerprint: '+digest+'\n')
    argv,cwd,kind=_resolve(str(snapshot),True)
    results=[]
    for length in (3,4):
        walls=set()
        def wall(x,y,d):
            if d=='N':key=(y%16)*66+x
            elif d=='S':key=((y+1)%16)*66+x
            elif d=='W':key=y*66+33+x
            else:key=y*66+33+(x+1)%32
            walls.add(key)
        for d in 'ES':wall(4,10,d)
        for d in 'NEW':wall(3,9,d)
        for d in 'SW':wall(3,10,d)
        parts=' '.join(f'4 {9-i}' for i in range(length))
        board=f'MAP 32 16\nMAP_NAME short_pearl_escape\nTILE_COUNT 2\nTILE 3 9 1 1\nTILE 3 10 1 1\nEDGE_COUNT {len(walls)}\n'+''.join(f'EDGE {k} 1 -1\n' for k in sorted(walls))+f'DRAGON_COUNT 2\nDRAGON 0 {length} {parts}\nDRAGON 1 2 20 12 19 12\nEND\n'
        pools={};live={};teams={};births={};records=[];deaths=[];errors=[]
        def spawn(i,init):
            team=next(l.split()[1] for l in init.decode().splitlines() if l.startswith('TEAM '))
            teams[i]=team
            if team=='A':
                if team not in pools:pools[team]=WasmPool(argv,cwd=str(cwd),key=f'0000000000000007-{team.lower()}')
                live[i]=SandboxBot(pools[team],init=init,name=str(i))
                births[i]=[]
        def reply(i,block):
            lines=block.decode().splitlines();r=int(lines[0].split()[1])
            if r>=6:raise Done()
            if teams[i]=='B':return b'MOVE E\n'
            n=int(next(l.split()[1] for l in lines if l.startswith('LENGTH ')))
            if i==0 and r==0:return b'MOVE S\n' # place the dragon at the corner when the pearls appear
            data=live[i].ask(block)
            if live[i].error:errors.append(str(live[i].error))
            action=next((l for l in data.decode().splitlines() if l.startswith(('MOVE ','SPLIT '))),'NONE')
            records.append({'id':i,'round':r,'length':n,'action':action})
            births[i].append((r,n,action))
            return data
        def death(i,r,why):
            deaths.append({'id':i,'round':r,'reason':why})
            b=live.pop(i,None)
            if b:b.stop()
        try:EngineModule().run(board.encode(),reply,death,spawn,on_notice=lambda s:None,seed=7)
        except Done:pass
        finally:
            for b in live.values():b.stop()
            for p in pools.values():p.close()
        original=[r for r in records if r['id']==0]
        assert [r['action'] for r in original[:3]]==['MOVE W','MOVE N','SPLIT 3'],original
        children=[i for i,rows in births.items() if i!=0 and rows and rows[0][0]==3 and rows[0][1]==3]
        assert children,records
        assert not any(d['id'] in children for d in deaths),deaths
        assert not errors,errors
        results.append({'initial_length':length,'turns':records,'deaths':deaths,'errors':errors,'child_ids':children})
    out.write_text(json.dumps(results,indent=2))
    for r in results:print(json.dumps(r))
