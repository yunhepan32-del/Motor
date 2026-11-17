"""HOST_ONLY system, schema, parameter and frozen-real compatibility QA."""
from pathlib import Path
import sys,json,hashlib,copy,tempfile
from ccs_memory import load_memory
from parse_stage6 import decode
from motor_parameters import calculate,validate_review,generate
H=Path(__file__).resolve().parents[1];R=H.parents[1];RESULT=H/'results/09/O2'

def words(p):
    m=load_memory(p);return [m[k] for k in range(0xC000,0xD000)]
def reject(f):
    try:f()
    except (ValueError,KeyError,TypeError):return
    raise AssertionError('Bad fixture accepted')
def main():
    cases={}
    for p in RESULT.glob('*.dat'):
        q=decode(words(p),True);cases[p.name]=dict(status=q['status'],faults=q['errorBits'])
    for folder in ['Stage6_R01_RAW_02/回传原件/24V_ON','Stage6_R01_RAW_02/回传原件/24V_OFF','Stage6_R02_CAL_01/回传原件']:
        E=globals().get('REALROOT',R/'实物记录/2026-11-17')/folder
        p=next((E/'原始数据').glob('*.dat'));q=decode(words(p));q['inputSHA256']=hashlib.sha256(p.read_bytes()).hexdigest().upper()
        assert q==json.loads((E/'解析结果/summary.json').read_text(encoding='utf-8-sig'));cases['legacy_'+folder]='exact JSON'
    w=words(RESULT/'HOST_ONLY_feedback_stable_unqualified.dat')
    for off,value in [(4,2),(14,1),(46,99),(47,128),(67,2),(66,1),(94,1),(58,1),(2437,0),(2496,0),(2564,65535),(3646,1)]:
        x=w[:];x[off]=value;reject(lambda:decode(x,True));cases['mutation_'+str(off)]='rejected'
    p=json.loads((H/'profile/HOST_ONLY_motor.json').read_text());o=calculate(p);assert not o['evidenceGranted'] and o['compatibleWithRetainedSymmetricSF']
    assert abs(o['gains']['kp_d_V_per_A']-2.0734511513692633)<1e-9
    for key,value in [('Rs',None),('Ld',float('nan')),('J',0),('flux',0),('rated_current',3),('counts_per_rev',3),('direction',0),('current_bandwidth_Hz',2000),('speed_bandwidth_Hz',100),('damping',0),('iq_slew_A_s',0),('build','0x26111701')]:
        bad=copy.deepcopy(p);bad[key]=value;reject(lambda:calculate(bad));cases['motor_'+key]='rejected'
    pending=json.loads((H/'profile/motor_profile.json').read_text());reject(lambda:calculate(pending));cases['actual_motor_pending']='blocked'
    with tempfile.TemporaryDirectory() as td:
        root=Path(td);f=root/'raw.dat';f.write_bytes(b'HOST_ONLY reviewed model fixture')
        approved=copy.deepcopy(p);approved.update(review_status='APPROVED',reviewer='HOST_ONLY',conditions='synthetic',raw_sources=[dict(path='raw.dat',sha256=hashlib.sha256(f.read_bytes()).hexdigest())])
        assert validate_review(approved,root);assert '0x26111702UL' in generate(approved)
        for key,value in [('parameter_convention','LINE_LINE'),('reviewer',None),('Ld',.0008),('raw_sources',[])]:
            bad=copy.deepcopy(approved);bad[key]=value;reject(lambda:validate_review(bad,root));cases['import_'+key]='rejected'
        approved['raw_sources'][0]['sha256']='0'*64;reject(lambda:validate_review(approved,root));cases['import_wrongsha']='rejected'
    result=dict(evidence='HOST_ONLY_AND_REAL_RAM_OFFLINE_COMPATIBILITY',result='PASS',checks=len(cases),cases=cases)
    globals().get('OUTPUT',H/'results/R03_parser_parameter_tests.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PASS',len(cases),'R03 parser/parameter/legacy checks; no hardware performance claim')
if __name__=='__main__':main()
