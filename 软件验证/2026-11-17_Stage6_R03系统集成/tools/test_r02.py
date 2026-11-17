"""HOST_ONLY exact statistics, legacy compatibility and calibration negatives."""
from pathlib import Path
import copy,hashlib,json,math,sys
from ccs_memory import load_memory
from parse_stage6 import decode
from calibrate_profile import fit,validate,header

H=Path(__file__).resolve().parent.parent
RESULT=H/'results/06/O2'


def words(path):
    m=load_memory(path);return [m[i] for i in range(0xC000,0xD000)]


def expect_error(action):
    try:action()
    except (ValueError,KeyError):return
    raise AssertionError('Negative fixture unexpectedly accepted')


def main():
    cases={}
    for path in RESULT.glob('*.dat'):
        r=decode(words(path),True);cases[path.name]=dict(status=r['status'],stats=r['statisticsSamples'])
    assert decode(words(RESULT/'HOST_ONLY_feedback_noisy.dat'),True)['calibration']['rejectBits']&8
    assert decode(words(RESULT/'HOST_ONLY_feedback_saturated.dat'),True)['statistics'][0]['saturated']==1
    path=RESULT/'HOST_ONLY_feedback_stable_unqualified.dat';w=words(path);r=decode(w,True)
    assert r['fullFeedbackCalibrationSuite'] and r['calibration']['complete'] and r['calibration']['rejectBits']==3
    assert not r['calibration']['approved'] and r['focCalls']==0 and all(c['sd']==0 for c in r['statistics'])
    assert len(r['rawWindows'])==128 and r['rawWindows'][64]['sequence']==4033
    r=decode(words(RESULT/'HOST_ONLY_feedback_low.dat'),True)
    expected=[80+(k%173) for k in range(1,4097)]
    assert r['statistics'][0]['sum']==sum(expected)
    numerator=len(expected)*sum(v*v for v in expected)-sum(expected)**2
    assert abs(r['statistics'][0]['variance']-numerator/4096**2)<1e-10
    r=decode(words(RESULT/'HOST_ONLY_feedback_eligible.dat'),True)
    assert r['calibration']['statisticalGatePassed'] and not r['calibration']['approved']
    mutations=[(2,0),(4,1),(14,1),(2432,1),(2432+1,1024),(2432+5,0),
               (2432+8,4097),(2432+64,0),(2432+128+4,65535),
               (2432+1184+30,1),(2432+1184+17,0),(384,999)]
    for i,(offset,value) in enumerate(mutations):
        bad=w[:];bad[offset]=value;expect_error(lambda:decode(bad,True));cases['mutation'+str(i)]='rejected'
    root=globals().get('R01ROOT',H.parents[1]/'实物记录/2026-11-17/Stage6_R01_RAW_02/回传原件')
    for cond in ['24V_ON','24V_OFF']:
        path=next((root/cond/'原始数据').glob('*.dat'));r=decode(words(path));r['inputSHA256']=hashlib.sha256(path.read_bytes()).hexdigest().upper()
        assert r==json.loads((root/cond/'解析结果/summary.json').read_text(encoding='utf-8'))
        cases['R01_'+cond]='exact JSON match'
    def point(raw,ref,role='fit'):
        return dict(raw=str(raw),reference=str(ref),role=role,powered='yes',synchronous='yes',raw_sha256='0'*64,instrument='HOST_ONLY_SYNTHETIC')
    points=[point(2100,5),point(2500,15),point(2900,25),point(2700,20,'holdout')]
    out=fit(points,'bus');assert abs(out['signed_gain']-.025)<1e-12 and abs(out['zero_count']-1900)<1e-9 and not out['evidenceGranted']
    current=[point(1900,-1),point(2000,0),point(2100,1),point(2050,.5,'holdout')]
    assert abs(fit(current,'IW')['signed_gain']-.01)<1e-12
    for key,val in [('powered','no'),('synchronous','no'),('reference','nan'),('raw','4095'),('raw_sha256','broken'),('instrument','')]:
        bad=copy.deepcopy(points);bad[0][key]=val;expect_error(lambda:fit(bad,'bus'));cases['fit_'+key]='rejected'
    expect_error(lambda:fit(points[:3],'bus'));expect_error(lambda:fit(points,'IU'))
    profile=json.loads((H/'profile/hardware_profile.json').read_text());assert validate(profile,H)==0
    assert header(profile,0)==(H/'src/s6_calibration_profile.h').read_text()
    import tempfile
    with tempfile.TemporaryDirectory() as td:
        root=Path(td);record=root/'raw.dat';record.write_bytes(b'HOST_ONLY calibration witness')
        source=dict(path='raw.dat',sha256=hashlib.sha256(record.read_bytes()).hexdigest())
        approved=copy.deepcopy(profile);ch=approved['channels']['IU']
        ch.update(review_status='APPROVED',reviewer='HOST',experiment='HOST',instrument='HOST',conditions='HOST',raw_sources=[source],gain=.001,zero_count=2048,verified_range=2.5,holdout_verified=True,gain_polarity_verified=True)
        assert validate(approved,root)==0
        for key,value in [('gain',float('nan')),('gain',0),('zero_count',5000),('verified_range',-1),('gain_polarity_verified',False)]:
            bad=copy.deepcopy(approved);bad['channels']['IU'][key]=value
            expect_error(lambda:validate(bad,root));cases['approved_invalid_'+key+str(value)]='rejected'
        for path,sha in [('../outside.dat',source['sha256']),('raw.dat','f'*64)]:
            bad=copy.deepcopy(approved);bad['channels']['IU']['raw_sources']=[dict(path=path,sha256=sha)]
            expect_error(lambda:validate(bad,root));cases['provenance_'+path+sha[:4]]='rejected'
    for key,value in [('gain',float('nan')),('gain',0),('zero_count',5000),('verified_range',-1)]:
        bad=copy.deepcopy(profile);ch=bad['channels']['IU'];ch.update(review_status='APPROVED',reviewer='HOST',experiment='HOST',instrument='HOST',conditions='HOST',raw_sources=[]);ch[key]=value
        expect_error(lambda:validate(bad,H));cases['profile_'+key+str(value)]='rejected'
    globals().get('OUTPUT',H/'results/R02_parser_calibration_tests.json').write_text(json.dumps(dict(evidence='HOST_ONLY',result='PASS',cases=cases),indent=2),encoding='utf-8')
    print('PASS',len(cases),'R02 fixture/negative/legacy checks; no hardware')


if __name__=='__main__':main()
