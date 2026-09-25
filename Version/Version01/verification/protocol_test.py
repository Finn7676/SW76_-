import subprocess
import sys
from pathlib import Path
binary=Path(sys.argv[1]).resolve()
def frame(r,x,y,msgs=(),echo=False):
 s=f'ROUND {r}\nDIR E\nLENGTH 3\nUNIT_COUNT 10\nNUM_MSGS {len(msgs)}\n'+''.join(f'{m}\n' for m in msgs)
 if echo:s+='ECHOES 1 1 0 0 0\n'
 for dy in range(-3,4):
  for dx in range(-3,4): s+=f'{(x+dx)%11} {(y+dy)%11} 0 -1\n'
 s+=f'DRAGON_BODIES 1\nA 7 {x} {y} E 1\n'
 s+=('. . . . . . .\n')*8
 for dy in range(-3,4):
  row=[]
  for dx in range(-3,5):
   # vertical edge stored by cell on its east; endpoints (5,5)E and (9,9)E
   row.append('23' if ((x+dx)%11,(y+dy)%11) in [(6,5),(10,9)] else '.')
  s+=' '.join(row)+'\n'
 return s
init='ID 7\nTEAM A\nMAP 11 11\nUNIT_LIMIT 64\n'
def run(s):
 p=subprocess.run([str(binary)],input=init+s+'ENDGAME\n',text=True,capture_output=True,timeout=5)
 assert p.returncode==0,p.stderr
 return p.stdout.split('ENDTURN\n')[:-1]
out=run(frame(1,5,5)+frame(2,10,9,echo=True))
assert 'PROTOCOL 3' in out[0] and 'MOVE E' in out[0],out
assert not any(line.startswith('SONAR N ') for line in out[0].splitlines()),out[0]
beams=[line.split() for line in out[1].splitlines() if line.startswith('SONAR')]
assert [b[1] for b in beams]==list('NESW'),beams
msg=int(beams[0][2]);assert msg>2**32 and ((msg>>40)&255)==0xB7 and ((msg>>38)&1)==1
peer=run(frame(3,5,5,[msg],echo=True))[0]
# A known portal remains a legal ordinary move; inspect relayed knowledge instead.
peer_msgs=[int(l.split()[2]) for l in peer.splitlines() if l.startswith('SONAR')]
assert peer_msgs and all(((m>>40)&255)==0xB7 and ((m>>38)&1) for m in peer_msgs),peer
assert len([l for l in peer.splitlines() if l.startswith('SONAR')])==4
print('PASS protocol 2 initial frame -> protocol 3 echoes + 64-bit sonar -> peer relays complete explored pair')
