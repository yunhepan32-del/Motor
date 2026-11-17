from pathlib import Path
import hashlib
import subprocess

H=Path(__file__).resolve().parent
R=H.parents[1]
B=R/'ccs_stage6_workspace/Stage6_SENSORED_FOC/CPU1_SENSORED_FOC_RAM'
T=Path('D:/ti/ccs2101/ccs/tools/compiler/ti-cgt-c2000_25.11.1.LTS/bin')
A=H/'artifacts_R03'
A.mkdir(exist_ok=False)
out=B/'Stage6_SENSORED_FOC.out'
for tool,args,name in [
    ('dis2000.exe',['-d',str(out)],'Stage6_SENSORED_FOC.disasm.txt'),
    ('ofd2000.exe',['--xml',str(out)],'Stage6_SENSORED_FOC.ofd.xml'),
    ('ofd2000.exe',['--func_info',str(out)],'function_info.txt'),
    ('ofd2000.exe',['--call_graph',str(out)],'call_graph.xml'),
    ('nm2000.exe',['-n',str(out)],'Stage6_SENSORED_FOC.symbols.txt')]:
    x=subprocess.run([str(T/tool)]+args,stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    (H/'logs'/('final_'+name+'.stderr.log')).write_bytes(x.stderr)
    assert x.returncode==0,(tool,x.stderr)
    (A/name).write_bytes(x.stdout)
for name in ['Stage6_SENSORED_FOC.out','Stage6_SENSORED_FOC.map','Stage6_SENSORED_FOC_linkInfo.xml']:
    (A/name).write_bytes((B/name).read_bytes())
print('FINAL OUT SHA256',hashlib.sha256(out.read_bytes()).hexdigest().upper())
