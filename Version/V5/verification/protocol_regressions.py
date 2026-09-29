"""Replay captured engine inputs against a freshly compiled bot."""
from pathlib import Path
import subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
flags=['-std=c++20','-O1','-g','-fsanitize=address,undefined'] if '--sanitize' in sys.argv else ['-std=c++20','-O2']
with tempfile.TemporaryDirectory(prefix='v4-protocol-check-') as tmp:
 binary=Path(tmp)/'bot'
 subprocess.run(['c++',*flags,str(root/'main.cpp'),'-o',str(binary)],check=True)
 # The historical pearl-priority case now reaches length 8 during early development:
 # balanced splitting must outrank its nearby pearl cluster.
 expected = {'dilemma_long_A_round0': 'SPLIT 2', 'dilemma_long_B_round0': 'SPLIT 2',
             'devil_released_pearl_priority': 'SPLIT 4', 'devil_screenshot_reconstruction': 'MOVE W'}
 for name in ['dilemma_round0','autarky_round0','devil_round38',*expected]:
  result=subprocess.run([str(binary)],input=(root/'verification/fixtures'/f'{name}.input').read_bytes(),capture_output=True,check=True)
  assert not result.stderr,result.stderr.decode(errors='replace')
  turns=[x for x in result.stdout.decode().split('ENDTURN') if x.strip()]
  actions=[x for x in turns[-1].splitlines() if x.startswith(('MOVE ','SPLIT '))]
  assert actions,(name,result.stdout.decode())
  if name=='devil_round38':assert actions[-1].startswith('SPLIT '),actions
  if name in expected:assert actions[-1]==expected[name],(name,actions)
  print(name,actions[-1])
