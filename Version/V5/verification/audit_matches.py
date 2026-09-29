import sys,json,pathlib,collections,time,concurrent.futures,os,shutil,tempfile,hashlib
from dataclasses import asdict
from unswbc.engine import EngineModule
from unswbc.bot import Pool,Bot
from unswbc.run import _resolve
root=pathlib.Path(os.environ.get('V4_AUDIT_OUT','/tmp/v4-audit-results'));root.mkdir(parents=True,exist_ok=True)
maps=pathlib.Path(__file__).resolve().parents[2]/'maps'
label,candidate,opponent=sys.argv[1:4];mode=sys.argv[4];limit=int(sys.argv[5]);names=sys.argv[6:] or [p.stem for p in sorted(maps.glob('*.map'))]
# The toolkit hashes .cpp files only. Snapshot headers too and embed their
# fingerprint so a header-only edit cannot silently reuse an old WASM binary.
snapshot_root=tempfile.TemporaryDirectory(prefix='v4-audit-build-')
def resolve_fresh(path,index):
 p=pathlib.Path(path).resolve()
 if not p.is_dir():return _resolve(str(p),mode=='wasm')
 dest=pathlib.Path(snapshot_root.name)/('bot-'+str(index));dest.mkdir()
 sources=[]
 for f in sorted(p.rglob('*')):
  rel=f.relative_to(p)
  if not f.is_file() or any(x.startswith('.') for x in rel.parts):continue
  if f.suffix not in ['.cpp','.cc','.c','.h','.hpp'] and rel.name!='bot.toml':continue
  target=dest/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(f,target)
  sources.append(target)
 digest=hashlib.sha256(b''.join(f.read_bytes() for f in sources)).hexdigest()
 cpp=next(f for f in sources if f.suffix in ['.cpp','.cc','.c'])
 with cpp.open('a') as fp:fp.write('\n// Full source fingerprint: '+digest+'\n')
 return _resolve(str(dest),mode=='wasm')
resolved=[resolve_fresh(p,i) for i,p in enumerate([candidate,opponent])]
if mode=='wasm':
 from unswbc.sandbox import WasmPool,SandboxBot
 Pool,Bot=WasmPool,SandboxBot
class Done(Exception):pass
def run(task):
 name,side=task;seed=15470468907592539593 if name=='devil' else 7
 pools={};live={};sides={};records=[];history={};last={};errors=[];deaths=[];counts=collections.Counter();peak=0;steps=0;missed_splits=[]
 for team,k in [('A',0 if side=='A' else 1),('B',1 if side=='A' else 0)]:
  argv,cwd,kind=resolved[k];pools[team]=Pool(argv,cwd=str(cwd),key=f'{seed:016x}-{team.lower()}')
 def spawn(i,init):
  team=next(l.split()[1] for l in init.decode().splitlines() if l.startswith('TEAM '));sides[i]=team;live[i]=Bot(pools[team],init=init,name=str(i));history[i]=init
 def reply(i,block):
  nonlocal peak,steps
  r=int(block.splitlines()[0].split()[1])
  if r>=limit:raise Done()
  bot=live[i];out=bot.ask(block);last[i]=(block.decode(),out.decode());history[i]+=block
  if sides[i]==side:
   steps+=1
   if bot.error:errors.append({'id':i,'round':r,'error':str(bot.error)})
   act=[l for l in out.decode().splitlines() if l.startswith(('MOVE ','SPLIT '))]
   counts[act[0].split()[0] if act else 'EMPTY']+=1
   if not act:
    info={l.split()[0]:int(l.split()[1]) for l in block.decode().splitlines() if l.startswith(('LENGTH ','UNIT_COUNT '))}
    if info['LENGTH']>=4 and info['UNIT_COUNT']<64 and r<400:missed_splits.append({'round':r,'id':i,**info})
   metrics=getattr(bot,'live',None)
   if metrics and metrics[0]:peak=max(peak,metrics[0])
   if r<=5:records.append({'id':i,'round':r,'input':block.decode(),'output':out.decode()})
  return out
 def death(i,r,why):
  if sides[i]==side:
   deaths.append({'id':i,'round':r,'reason':why,'input':last.get(i,('',''))[0],'output':last.get(i,('',''))[1]})
   if (name=='devil' and i==0 and r==38) or (r==0 and why=='A') or why=='S':
    (root/f'{label}-{name}-{side}-{i}-{r}.input').write_bytes(history[i]+b'ENDGAME\n')
  b=live.pop(i,None)
  if b:b.stop()
  history.pop(i,None)
 start=time.monotonic();engine=EngineModule();result=None
 try:
  result=asdict(engine.run((maps/f'{name}.map').read_bytes(),reply,death,spawn,on_notice=lambda s:None,seed=seed))
  if name=='devil':(root/f'{label}-{name}-{side}.replay').write_bytes(engine.replay(candidate if side=='A' else opponent,opponent if side=='A' else candidate))
 except Done:pass
 finally:
  for b in live.values():b.stop()
  for p in pools.values():p.close()
 data={'map':name,'side':side,'seed':seed,'result':result,'errors':errors,'deaths':deaths,'actions':dict(counts),'turns':steps,'peak_points':peak,'missed_splits':missed_splits,'seconds':round(time.monotonic()-start,2)}
 (root/f'{label}-{name}-{side}-early.json').write_text(json.dumps(records))
 (root/f'{label}-{name}-{side}.json').write_text(json.dumps(data,indent=2))
 print(json.dumps({k:v for k,v in data.items() if k not in ['deaths','errors','missed_splits']}|{'errors':len(errors),'missed_splits':len(missed_splits),'deaths':dict(collections.Counter(d['reason'] for d in deaths))}),flush=True)
 return data
with concurrent.futures.ThreadPoolExecutor(max_workers=2 if mode=='native' and limit>=500 else 1) as ex:
 results=list(ex.map(run,[(n,s) for n in names for s in ['A','B']]))
(root/f'{label}-summary.json').write_text(json.dumps(results,indent=2))

snapshot_root.cleanup()
