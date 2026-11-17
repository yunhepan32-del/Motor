"""One frozen R03 review bundle, verified against the actual built project."""
from pathlib import Path
import hashlib,json,shutil,subprocess,sys,zipfile

H=Path(__file__).resolve().parent;R=H.parents[1]
P=R/'ccs_stage6_workspace/Stage6_SENSORED_FOC'
D=R/'网页版交接/Stage6_SENSORED_FOC_R03_2026-11-17'
OLD=R/'网页版交接/Stage6_SENSORED_FOC_R02_2026-11-17'

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def copy(a,b):
    b.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(a,b)
def tree(a,b,exclude=()):
    for p in a.rglob('*'):
        if p.is_file() and not any(x in p.relative_to(a).parts for x in exclude):copy(p,b/p.relative_to(a))

def main():
    D.mkdir(exist_ok=False)
    for f in H.glob('*.md'):copy(f,D/f.name)
    tree(P,D/'工程源码/Stage6_SENSORED_FOC',exclude=('CPU1_SENSORED_FOC_RAM','.theia'))
    tree(H/'artifacts_R03',D/'构建产物')
    # Exact build option expansion makes target toolchain configuration reviewable.
    for n in ['ccsIncludes.opt','makefile','objects.mk','sources.mk']:
        copy(P/'CPU1_SENSORED_FOC_RAM'/n,D/'构建设置'/n)
    tree(H/'target_object_review',D/'独立目标后端对象_未链接')
    for n in ['08_build.log','build_08_commands.json','target_object_01.log','target_object_01_command.json']:
        copy(H/'logs'/n,D/'构建日志'/n)
    tree(H/'logs/host_09',D/'构建日志/host_09')
    tree(H/'audit',D/'审计')
    copy(H/'完整源码差异.diff',D/'完整源码差异.diff')
    copy(H/'R03_用户任务原文.txt',D/'R03_用户任务原文.txt')
    tree(H/'host',D/'本地主机证据/host')
    tree(H/'src',D/'本地主机证据/src')
    tree(H/'profile',D/'校准/profile')
    for p in (OLD/'校准/profile').glob('*'):copy(p,D/'校准/profile'/p.name)
    for p in (H/'tools').glob('*.py'):
        if p.name!='test_r02.py':copy(p,D/'解析工具'/p.name)
    tree(H/'results/09',D/'本地主机证据/results/09',exclude=('test.exe',))
    copy(H/'results/R03_parser_parameter_tests.json',D/'本地主机证据/results/R03_parser_parameter_tests.json')
    copy(H/'results/HOST_ONLY_tuning.json',D/'本地主机证据/results/HOST_ONLY_tuning.json')
    tree(H/'results/R02_correlations',D/'结果/R02_correlations')
    tree(OLD/'硬件来源',D/'硬件来源')
    tree(OLD/'官方参考_不编译',D/'官方参考_不编译')
    for name in ['RAM布局与评分.md','03_真实硬件与校准条件.md','RAW全采样统计与偏置校准.md','校准profile与离线工具.md']:
        copy(OLD/name,D/'历史R02参考'/name)
    for name in ['Stage6_R01_RAW_02','Stage6_R02_CAL_01']:
        tree(R/'实物记录/2026-11-17'/name,D/'历史真实证据'/name)
    for name in ['Stage6_SENSORED_FOC.out','Stage6_SENSORED_FOC.map']:
        copy(H/'R02冻结工程/CPU1_SENSORED_FOC_RAM'/name,D/'历史R02冻结镜像'/name)
    for name in ['make_documents.py','audit_stage6.py','build.py','extract_final.py','verify.py','package_r03.py']:
        copy(H/name,D/'开发复现脚本'/name)
    (D/'运行包内自检.py').write_text('''from pathlib import Path
import sys,tempfile
sys.dont_write_bytecode=True
r=Path(__file__).resolve().parent
sys.path.insert(0,str(r/'解析工具'))
import test_r03
with tempfile.TemporaryDirectory() as td:
 test_r03.H=r/'校准'
 test_r03.RESULT=r/'本地主机证据/results/09/O2'
 test_r03.REALROOT=r/'历史真实证据'
 test_r03.OUTPUT=Path(td)/'result.json'
 test_r03.main()
''',encoding='utf-8')
    (D/'校验包.py').write_text('''from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parent
seen=set()
for line in (r/'SHA256SUMS').read_text(encoding='utf-8').splitlines():
 expected,name=line.split('  ',1);p=(r/name).resolve()
 assert p.is_relative_to(r.resolve()) and p.is_file(),name
 assert hashlib.sha256(p.read_bytes()).hexdigest().upper()==expected,name
 seen.add(name)
actual={p.relative_to(r).as_posix() for p in r.rglob('*') if p.is_file() and p!=r/'SHA256SUMS'}
assert seen==actual,(actual-seen,seen-actual)
m=json.loads((r/'校验清单.json').read_text(encoding='utf-8'))
assert hashlib.sha256((r/'构建产物/Stage6_SENSORED_FOC.out').read_bytes()).hexdigest().upper()==m['OUT_SHA256']
print('PASS',len(seen),'files; exact frozen OUT identity; offline only')
''',encoding='utf-8')
    (D/'本地主机证据/rebuild_host.py').write_text('''from pathlib import Path
import sys,subprocess,json
h=Path(__file__).resolve().parent;p=h.parents[0]/'工程源码/Stage6_SENSORED_FOC'
dest=Path(sys.argv[1]).resolve();dest.mkdir(parents=True,exist_ok=False)
gcc=sys.argv[2] if len(sys.argv)>2 else r'D:\\Programs\\mingw64\\bin\\gcc.exe'
for opt in ['O0','O2']:
 d=dest/opt;d.mkdir()
 cmd=[gcc,'-std=c99','-'+opt,'-Wall','-Wextra','-Werror','-Wno-unknown-pragmas','-fno-fast-math','-ffp-contract=off','-fexcess-precision=standard','-D__asm(x)=','-include',str(h/'host/host_compat.h'),'-I'+str(h/'src'),'-I'+str(p),'-I'+str(p/'libraries/utilities/types/include')]
 cmd += [str(p/n) for n in ['sensored_foc.c','encoder_feedback.c']]
 cmd += [str(h/'src'/n) for n in ['s6_profile.c','s6_control.c','s6_feedback.c','s6_speed.c','s6_stats.c','s6_pwm.c','s6_system.c','s6_motor.c','s6_alignment.c']]
 cmd += [str(h/'host/test_stage6.c'),'-lm','-o',str(d/'test.exe')]
 for label,c in [('build',cmd),('run',[str(d/'test.exe')])]:
  x=subprocess.run(c,cwd=d,capture_output=True);(d/(label+'.log')).write_bytes(x.stdout+x.stderr);assert not x.returncode,(opt,label)
 for f in (h/'results/09'/opt).glob('*'):
  if f.suffix in ['.dat','.csv']:assert f.read_bytes()==(d/f.name).read_bytes(),(opt,f.name)
print('PASS O0/O2 rebuilt exactly; HOST_ONLY, no hardware')
''',encoding='utf-8')
    # Run package in isolation so original absolute paths cannot mask omissions.
    selftest=subprocess.run([sys.executable,'-B','-X','utf8',str(D/'运行包内自检.py')],capture_output=True)
    (D/'包内自检执行.log').write_bytes(selftest.stdout+selftest.stderr)
    assert not selftest.returncode,(selftest.stdout,selftest.stderr)
    rebuilt=H/'results/package_rebuild_02'
    x=subprocess.run([sys.executable,'-B','-X','utf8',str(D/'本地主机证据/rebuild_host.py'),str(rebuilt)],capture_output=True)
    (D/'包内主机重建.log').write_bytes(x.stdout+x.stderr)
    assert not x.returncode,(x.stdout,x.stderr)
    identity=json.loads((H/'audit/image_identity.json').read_text())
    assert sha(D/'构建产物/Stage6_SENSORED_FOC.out')==sha(P/'CPU1_SENSORED_FOC_RAM/Stage6_SENSORED_FOC.out')==identity['OUT_SHA256']
    for f in P.rglob('*'):
        rel=f.relative_to(P)
        if f.is_file() and not any(x in rel.parts for x in ['CPU1_SENSORED_FOC_RAM','.theia']):
            assert sha(f)==sha(D/'工程源码/Stage6_SENSORED_FOC'/rel),rel
    checklist=dict(status='LOCAL_SOFTWARE_AND_BUILD_VERIFIED',build='0x26111702',layout=3,
        OUT_SHA256=identity['OUT_SHA256'],ready=identity['S6_readyStop'],done=identity['S6_doneStop'],
        commandAddress='0x0400',recommendedHardwareCommand=None,RAM=identity['export'],
        GCC='O0/O2 strict; 33 DAT/CSV byte-identical; package rebuilt exactly',pythonChecks=64,
        TI='25.11.1.LTS FPU32 EABI O2 strict fp_reassoc=off; zero errors/warnings',
        targetBackend='Separately object compiled; excluded from loadable OUT',
        chipTestPerformed=False,powerEnabled=False,hardwareFOCVerified=False,strictWCETVerified=False,
        packageSelftest='PASS',sourcesMatchActualProject=True,historyFilesPreserved=10343)
    (D/'校验清单.json').write_text(json.dumps(checklist,ensure_ascii=False,indent=2),encoding='utf-8')
    files=sorted(p for p in D.rglob('*') if p.is_file())
    (D/'SHA256SUMS').write_text(''.join(sha(p)+'  '+p.relative_to(D).as_posix()+'\n' for p in files),encoding='utf-8')
    x=subprocess.run([sys.executable,'-B','-X','utf8',str(D/'校验包.py')],capture_output=True)
    assert not x.returncode,(x.stdout,x.stderr)
    Z=D.with_suffix('.zip');assert not Z.exists()
    with zipfile.ZipFile(Z,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=7) as z:
        for p in sorted(D.rglob('*')):
            if p.is_file():z.write(p,(Path(D.name)/p.relative_to(D)).as_posix())
    with zipfile.ZipFile(Z) as z:
        assert z.testzip() is None
        for p in D.rglob('*'):
            if p.is_file():assert z.read((Path(D.name)/p.relative_to(D)).as_posix())==p.read_bytes()
    Z.with_suffix('.zip.sha256').write_text(sha(Z)+'  '+Z.name+'\n',encoding='utf-8')
    print(x.stdout.decode());print('FINAL ZIP',Z);print('ZIP SHA256',sha(Z));print('bytes',Z.stat().st_size)

if __name__=='__main__':main()
