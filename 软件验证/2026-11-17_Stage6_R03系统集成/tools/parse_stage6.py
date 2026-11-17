"""R03 layout3 and immutable R01/R02 backward-compatible decoder."""
from pathlib import Path
import argparse,json,hashlib,csv
from ccs_memory import load_memory
from parse_stage6_r02 import decode as previous

def decode(words,host_only=False):
    if len(words)!=4096:raise ValueError('4096 x16-bit words required')
    u=lambda k:words[k]|words[k+1]<<16
    if u(2)!=0x26111702:return previous(words,host_only)
    if words[4]!=3:raise ValueError('R03 layout must be 3')
    if words[46] not in range(2,13) and words[6]!=0:raise ValueError('Invalid lifecycle')
    if words[47]&~127 or words[67] or words[14] or words[66]:raise ValueError('Locked backend / qualifier export mismatch')
    w=words[:];w[2:5]=[0x1701,0x2611,2]
    r=previous(w,host_only);r.update(build='0x26111702',layout=3)
    r['system']=dict(state=words[46] or 2,qualifications=words[47],denied=words[55],PWMBackend='LOCKED',RAMCommits=u(58),executorFirstError=u(94),physicalPWMCommitted=False,physicalPowerAuthorized=False)
    if u(94) and not u(22):raise ValueError('PWM error not reflected in terminal faults')
    if words[6]==2 and u(58)!=u(68):raise ValueError('RAM PWM transaction coverage mismatch')
    r['limitations']+=['R03 integrates lifecycle and RAM PWM transactions; target backend is isolated and excluded.','No R03 chip timing or physical control qualification is inferred.']
    return r

def main():
    ap=argparse.ArgumentParser();ap.add_argument('input',type=Path);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--host-only',action='store_true');a=ap.parse_args()
    m=load_memory(a.input)
    if set(m)!=set(range(0xC000,0xD000)):raise ValueError('Exact C000..CFFF word addressed export required')
    w=[m[k] for k in range(0xC000,0xD000)];r=decode(w,a.host_only);r['inputSHA256']=hashlib.sha256(a.input.read_bytes()).hexdigest().upper()
    a.out.mkdir(parents=True,exist_ok=False);(a.out/'raw_words.json').write_text(json.dumps(w),encoding='utf-8');(a.out/'summary.json').write_text(json.dumps(r,ensure_ascii=False,indent=2,allow_nan=False),encoding='utf-8')
    for name,key in [('records','records'),('statistics','statistics'),('raw_windows','rawWindows')]:
        rows=r.get(key,[])
        if key=='statistics':rows=[dict(channel=ch,**v) for ch,v in zip(['IU','IV','IW','VBUS'],rows)]
        if not rows:continue
        with (a.out/(name+'.csv')).open('w',encoding='utf-8-sig',newline='') as f:
            wr=csv.DictWriter(f,fieldnames=list(rows[0]));wr.writeheader()
            for row in rows:wr.writerow({k:'N/A' if v is None else json.dumps(v) if isinstance(v,(list,dict)) else v for k,v in row.items()})
    (a.out/'report.md').write_text(f"# Stage6 offline report\n\n{r['build']}/layout{r['layout']}\n\nstatus={r['status']},processed={r['processed']},FOC={r['focSuccess']}/{r['focCalls']},fault={r['errorBits']}\n\nLocked physical output. Host model and local build are not hardware control verification.\n",encoding='utf-8')
    print(r['build'],r['evidence'],'status',r['status'])
if __name__=='__main__':main()
