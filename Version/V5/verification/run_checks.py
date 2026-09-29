"""Build and run every C++ regression without adding submission source files."""
from pathlib import Path
import concurrent.futures,subprocess,tempfile,sys
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='v4-checks-') as temporary:
 def check(source):
  output=Path(temporary)/source.name
  built=subprocess.run(['c++','-std=c++20','-O2','-x','c++','-I'+str(root),str(source),'-o',str(output)],capture_output=True,text=True)
  if built.returncode:return source.name,built.returncode,built.stderr
  result=subprocess.run([str(output)],capture_output=True,text=True)
  return source.name,result.returncode,result.stdout+result.stderr
 with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
  results=list(pool.map(check,sorted((root/'verification').glob('*.cpp.txt'))))
 for name,code,detail in results:
  print(('PASS ' if code==0 else 'FAIL ')+name)
  if code:print(detail)
 if any(code for _,code,_ in results):sys.exit(1)
