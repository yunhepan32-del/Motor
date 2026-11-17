"""Offline provenance, actual image and preservation checks; never a debugger."""
from pathlib import Path
import difflib
import hashlib
import json
import re
import xml.etree.ElementTree as ET

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
PROJECT=ROOT/'ccs_stage6_workspace/Stage6_SENSORED_FOC'
BASE=HERE/'R02冻结工程'
ART=HERE/'artifacts_R03'


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest().upper()


def main():
    before=json.loads((HERE/'audit/protected_before.json').read_text(encoding='utf-8'))
    changes=[n for n,h in before.items() if not (ROOT/n).is_file() or sha(ROOT/n)!=h]
    (HERE/'audit/history_preservation.json').write_text(json.dumps(dict(
        checked=len(before), changed=changes, result='PASS' if not changes else 'FAIL'),indent=2),encoding='utf-8')
    assert not changes, changes
    names=['sensored_foc.c','sensored_foc.h','encoder_feedback.c','encoder_feedback.h',
           'board_hal.h','src_device/f28003x_wildfire_locked_ram.cmd',
           'libraries/transforms/clarke/clarke.h','libraries/transforms/park/park.h',
           'libraries/transforms/ipark/ipark.h','libraries/transforms/svgen/svgen.h',
           'libraries/control/pi/pi.h','s6_port.c','wildfire_board.c','wildfire_lock.c',
           'TMS320F280039C.ccxml']
    names=sorted(set(names+[x.relative_to(PROJECT).as_posix() for x in PROJECT.rglob('*.ccxml')]))
    math={n:dict(sha256=sha(PROJECT/n),byteIdentical=(BASE/n).read_bytes()==(PROJECT/n).read_bytes()) for n in names}
    assert all(x['byteIdentical'] for x in math.values())
    (HERE/'audit/retained_modules.json').write_text(json.dumps(math,indent=2),encoding='utf-8')
    for f in (HERE/'src').glob('*'):
        assert f.read_bytes()==(PROJECT/f.name).read_bytes(),f
    out='Stage6_SENSORED_FOC.out'
    assert sha(ART/out)==sha(PROJECT/'CPU1_SENSORED_FOC_RAM'/out)
    asm=(ART/'Stage6_SENSORED_FOC.disasm.txt').read_text()
    symbols=(ART/'Stage6_SENSORED_FOC.symbols.txt').read_text()
    def address(name):
        return int(re.search(r'^([0-9a-fA-F]+)\s+[TtBb]\s+'+re.escape(name)+r'$',symbols,re.M)[1],16)
    def code(name):
        start=address(name)
        all_addresses=[int(x.findtext('address'),16) for x in ET.parse(ART/'call_graph.xml').findall('.//call_graph/function') if x.findtext('address')]
        end=min(x for x in all_addresses if x>start)
        return '\n'.join(line for line in asm.splitlines() if re.match(r'^[0-9a-f]{8}',line) and start<=int(line[:8],16)<end)
    checks={}
    for n in ['S6_readyStop','S6_doneStop']:
        s=code(n)
        assert all(x in s for x in ['7700','ff69','0006'])
        checks[n]=dict(address=f'0x{address(n):04X}',assembly=s)
    checks['command']=f'0x{address("gStage6Command"):04X}'
    checks['export']=dict(symbol='gStage6',start=f'0x{address("gStage6"):04X}',end='0xCFFF',words=4096)
    assert address('gStage6')==0xC000
    stage=code('S6P_stagePWM')
    assert 'MOV' not in '\n'.join(x for x in stage.splitlines() if '*+' in x and 'MOV' in x)
    assert not re.search(r'\b(?:motor1CtrlISR|ESMO_run|IRQ_poll|IRQ_onISR|S6T_configure|S6T_submit|S6T_observeLoad)\b',symbols)
    assert 's6_pwm_target.obj' not in (ART/'Stage6_SENSORED_FOC.map').read_text()
    checks['target_backend_excluded']=True
    checks['powerAllowedAssembly']=code('S6_powerAllowed') if re.search(r'\bS6_powerAllowed$',symbols,re.M) else 'Unreferenced function eliminated; source returns 0; target backend absent'
    checks['controlQualifiedAssembly']=code('Wildfire_controlQualified') if re.search(r'\bWildfire_controlQualified$',symbols,re.M) else 'Unreferenced function eliminated; source returns false; no enable provider'
    checks['RAM_stage_no_MMIO_write']=stage
    checks['main']=code('main')
    checks['legacy_estimator_ISR_absent']=True
    checks['OUT_SHA256']=sha(ART/out)
    (HERE/'audit/image_identity.json').write_text(json.dumps(checks,indent=2),encoding='utf-8')
    # Compiler call graph stack quantities are words. Duplicated static names
    # conservatively choose the largest local stack and union their callees.
    funcs={}
    for f in ET.parse(ART/'call_graph.xml').findall('.//call_graph/function'):
        n=f.findtext('name')
        if not n:continue
        v=funcs.setdefault(n,dict(stack=0,callees=set(),incomplete=False))
        v['stack']=max(v['stack'],int(f.findtext('stack_size') or '0',0))
        v['callees'].update(c.text for c in f.findall('callee') if c.text)
        v['incomplete']|=(f.findtext('has_stack_usage_info')!='true' or
                          f.findtext('makes_indirect_call')=='true' or
                          any(not c.text for c in f.findall('callee')))
    def peak(n,parents=()):
        assert n not in parents,('recursion',n)
        assert n in funcs,('missing function',n)
        v=funcs[n]
        children=[peak(c,parents+(n,)) for c in v['callees']]
        top=max(children,key=lambda x:x['words'],default=dict(words=0,path=[]))
        return dict(words=v['stack']+top['words'],path=[n]+top['path'])
    peaks={n:peak(n) for n in ['S6_appInit','Stage6_adcB1ISR','S6_poll']}
    memory=[]
    for l in (ART/'Stage6_SENSORED_FOC.map').read_text().splitlines():
        m=re.match(r'\s+(RAMM0S|RAMM1D|RAMLSnP|RAMENC|RAMGSC|RAMGSF|RAMGSD)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+([0-9a-f]+)',l)
        if m:memory.append(dict(name=m[1],origin=m[2],length=int(m[3],16),used=int(m[4],16),unused=int(m[5],16)))
    resources=dict(memory=memory,compilerStackPaths=peaks,
                   allocatedStackWords=728,hardwareContextAllowanceWords=14,
                   alignmentAllowanceWords=2,callerAllowanceWords=4,
                   conservativeRuntimeEstimateWords=peaks['Stage6_adcB1ISR']['words']+peaks['S6_poll']['words']+20,
                   limitation='Static estimate; nonnested ADCB1 only. NMI and independent runtime high-water not verified.',
                   incompleteFunctions=[n for n,v in funcs.items() if v['incomplete']])
    (HERE/'audit/resources.json').write_text(json.dumps(resources,indent=2),encoding='utf-8')
    selected=set(f.name for f in PROJECT.iterdir() if f.is_file())
    selected.update(['src_device/f28003x_wildfire_locked_ram.cmd','target_review_only/s6_pwm_target.c'])
    selected.update(f.name for f in BASE.iterdir() if f.is_file())
    diff=[];changes=[]
    for n in sorted(selected):
        a=(BASE/n).read_text(encoding='utf-8-sig',errors='replace').splitlines(True) if (BASE/n).is_file() else []
        b=(PROJECT/n).read_text(encoding='utf-8-sig',errors='replace').splitlines(True) if (PROJECT/n).is_file() else []
        if a!=b:
            changes.append(n)
            diff.extend(difflib.unified_diff(a,b,fromfile='Stage6_R02/'+n,tofile='Stage6_R03/'+n))
    (HERE/'完整源码差异.diff').write_text(''.join(diff),encoding='utf-8')
    (HERE/'audit/changed_files.json').write_text(json.dumps(changes,indent=2),encoding='utf-8')
    print(json.dumps(dict(preservedFiles=len(before),retainedModules=len(math),
                         outSHA=checks['OUT_SHA256'],identity=checks['export'],resources=resources),indent=2))


if __name__=='__main__':
    main()
