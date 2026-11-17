from pathlib import Path
import sys,subprocess,json
H=Path(__file__).resolve().parent;R=H.parents[1];P=R/'ccs_stage6_workspace/Stage6_SENSORED_FOC'
attempt=sys.argv[1];log=H/'logs'/('host_'+attempt);log.mkdir(exist_ok=False);commands=[]
for opt in ['O0','O2']:
 dest=H/'results'/attempt/opt;dest.mkdir(parents=True,exist_ok=False)
 cmd=[r'D:\Programs\mingw64\bin\gcc.exe','-std=c99','-'+opt,'-Wall','-Wextra','-Werror','-Wno-unknown-pragmas','-fno-fast-math','-ffp-contract=off','-fexcess-precision=standard','-D__asm(x)=','-include',str(H/'host/host_compat.h'),'-I'+str(H/'src'),'-I'+str(P),'-I'+str(P/'libraries/utilities/types/include')]
 cmd +=[str(P/n) for n in ['sensored_foc.c','encoder_feedback.c']]
 cmd +=[str(H/'src'/n) for n in ['s6_profile.c','s6_control.c','s6_feedback.c','s6_speed.c','s6_stats.c','s6_pwm.c','s6_system.c','s6_motor.c','s6_alignment.c']]
 cmd +=[str(H/'host/test_stage6.c'),'-lm','-o',str(dest/'test.exe')]
 for label,c in [('build',cmd),('run',[str(dest/'test.exe')])]:
  t=subprocess.run(c,cwd=dest,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
  (log/(opt+'_'+label+'.log')).write_bytes(t.stdout);commands.append(dict(command=c,cwd=str(dest),exit=t.returncode))
  (log/'commands.json').write_text(json.dumps(commands,indent=2),encoding='utf-8')
  print(opt,label,t.returncode,t.stdout.decode(errors='replace')[-4500:],flush=True)
  if t.returncode:sys.exit(t.returncode)
comp={f.name:f.read_bytes()==(H/'results'/attempt/'O2'/f.name).read_bytes() for f in (H/'results'/attempt/'O0').iterdir() if f.suffix in ['.dat','.csv']}
assert all(comp.values()),comp
(log/'O0O2_comparison.json').write_text(json.dumps(dict(evidence='HOST_ONLY',byteIdentical=comp),indent=2),encoding='utf-8')
