from pathlib import Path
import json,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
WORK=HERE/'ccs_build_workspace';P=ROOT/'ccs_stage6_workspace/Stage6_SENSORED_FOC'
CLI=r'D:\ti\ccs2101\ccs\eclipse\ccs-server-cli.bat'
attempt=sys.argv[1] if len(sys.argv)>1 else '01'
history=[]
for label,args in [('import',['-application','projectImport','-ccs.location',str(P)]),('build',['-application','projectBuild','-ccs.projects',P.name,'-ccs.configuration','CPU1_SENSORED_FOC_RAM','-ccs.buildType','full','-ccs.listProblems'])]:
 if label=='import' and len(sys.argv)>2:continue
 cmd=['cmd.exe','/d','/c',CLI,'-workspace',str(WORK)]+args
 log=HERE/'logs'/(attempt+'_'+label+'.log');assert not log.exists()
 with log.open('wb') as f:r=subprocess.run(cmd,cwd=ROOT,stdout=f,stderr=subprocess.STDOUT)
 history.append(dict(command=cmd,exit=r.returncode));(HERE/'logs'/('build_'+attempt+'_commands.json')).write_text(json.dumps(history,indent=2),encoding='utf-8')
 print(label,r.returncode,flush=True)
 if label=='build':
  content=log.read_text(encoding='utf-8',errors='replace')
  assert '0 errors, 0 warnings' in content and 'Finished building target:' in content and 'was not found' not in content,'No successful actual project build; inspect '+str(log)
 if r.returncode:print(log.read_text(encoding='utf-8',errors='replace')[-14000:]);sys.exit(r.returncode)
