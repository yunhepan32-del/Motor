"""Offline R02 layout 2 decoder; exact R01 layout 1 compatibility retained."""
from pathlib import Path
import argparse,csv,hashlib,json,math,struct
from ccs_memory import load_memory
from parse_stage6_r01 import decode as r01_decode

BUILD=0x26111701
EXT=2432


def decode(words,host_only=False):
    if len(words)!=4096:raise ValueError('4096 x16-bit words required')
    u=lambda i:words[i]|words[i+1]<<16
    f=lambda i:struct.unpack('<f',struct.pack('<I',u(i)))[0]
    if u(2)==0x26111608 and words[4]==1:
        return r01_decode(words,host_only)
    if (u(2),words[4])!=(BUILD,2):raise ValueError('Unsupported build/layout')
    copied=words[:];copied[2:5]=[0x1608,0x2611,1]
    result=r01_decode(copied,host_only)
    result.update(build='0x26111701',layout=2)
    h=EXT
    if words[h:h+5]!=[2,512,8,128,32]:raise ValueError('Statistics layout mismatch')
    n=u(h+8)
    if n>4096 or words[h+5] not in [0,1]:raise ValueError('Invalid statistics count/frozen state')
    if words[10] and not words[h+5]:raise ValueError('Finalized data without frozen statistics')
    if n>u(16):raise ValueError('Statistics exceed processed IRQs')
    if result['status']==2 and words[8]==6 and n!=u(16):raise ValueError('Successful suite omitted sample statistics')
    if words[8]!=6 and n:raise ValueError('Statistics falsely attached to other control mode')
    def cell(b):
        count,total,square=u(b),u(b+2),u(b+4)
        lo,hi,sat,jumps=words[b+6:b+10]
        if count and (lo>hi or hi>4095 or total<count*lo or total>count*hi or sat>count or jumps>count):
            raise ValueError('Invalid ADC statistics bounds')
        return dict(count=count,sum=total,squareCentered=square,min=lo if count else None,
                    max=hi if count else None,pp=hi-lo if count else None,
                    saturated=sat,validCount=count-sat,jumpsAbove32=jumps,
                    mean=total/count if count else None)
    totals=[cell(h+64+16*j) for j in range(4)]
    segments=[[cell(h+128+64*k+16*j) for j in range(4)] for k in range(8)]
    for j in range(4):
        t=totals[j];items=[segments[k][j] for k in range(8)]
        if result['status']==2 and t['count']!=n:raise ValueError('Channel sample coverage mismatch')
        for key in ['count','sum','saturated','jumpsAbove32']:
            if sum(x[key] for x in items)!=t[key]:raise ValueError('Segment/global accumulation mismatch')
        for k,x in enumerate(items):
            if x['count']>512 or result['status']==2 and x['count']!=min(max(n-512*k,0),512):
                raise ValueError('Segment coverage mismatch')
            if x['squareCentered']>x['count']*4194304:raise ValueError('Square accumulator exceeds proven bound')
            m=x['count'];v=m*x['squareCentered']-(x['sum']-2048*m)**2
            if v<0 or m and 4*v>m*m*x['pp']**2:raise ValueError('Impossible segment variance')
            x['variance']=v/(m*m) if m else None
            x['sd']=math.sqrt(x['variance']) if m else None
        square=sum(x['squareCentered'] for x in items);m=t['count'];v=m*square-(t['sum']-2048*m)**2
        if v<0 or m and 4*v>m*m*t['pp']**2:raise ValueError('Impossible total variance')
        t['squareCentered']=square;t['variance']=v/(m*m) if m else None
        t['sd']=math.sqrt(t['variance']) if m else None
        used=[x for x in items if x['count']]
        t['segmentMeanChange']=used[-1]['mean']-used[0]['mean'] if len(used)>1 else None
    window=[]
    for k in range(min(n,64)):
        window.append(dict(sequence=k+1,window='startup',raw=words[h+640+4*k:h+644+4*k]))
    for seq in range(max(65,n-63),n+1):
        slot=64+((seq-1)&63)
        window.append(dict(sequence=seq,window='tail',raw=words[h+640+4*slot:h+644+4*slot]))
    an=h+1152;anomaly=None
    if words[an]:
        if words[an]!=1 or not 1<=u(an+2)<=max(n,1) or words[an+4]>3:
            raise ValueError('Invalid first anomaly ownership')
        anomaly=dict(reason=words[an+1],sequence=u(an+2),channel=words[an+4],
                     raw=words[an+5],previous=words[an+6],allRaw=words[an+8:an+12])
    b=h+1184;count=u(b);reject=u(b+22)
    if count>2048 or words[b+30] or words[b+28]>1 or words[b+29]>1:
        raise ValueError('Invalid calibration/approval state')
    if count!=min(n,2048):raise ValueError('Candidate window coverage mismatch')
    candidate=[f(b+16+2*j) for j in range(3)] if words[b+28] else None
    if words[b+28] and count!=2048:raise ValueError('Incomplete candidate falsely marked complete')
    if candidate and any(not math.isfinite(v) or abs(v-u(b+2+2*j)/2048)>0.00025 for j,v in enumerate(candidate)):
        raise ValueError('Candidate mean differs from original sum')
    labels=[name for bit,name in [(1,'INTERFACE_NOT_REVIEWED'),(2,'ZERO_CURRENT_NOT_REVIEWED'),
             (4,'RAW_RANGE'),(8,'SPAN_ABOVE_32'),(16,'SATURATION'),(32,'INCOMPLETE'),(64,'ACCUMULATOR_OVERFLOW')] if reject&bit]
    cal=dict(count=count,sum=[u(b+2+2*j) for j in range(3)],min=words[b+8:b+11],max=words[b+11:b+14],
             candidate=candidate,complete=bool(words[b+28]),eligible=bool(words[b+29]),approved=False,
             rejectBits=reject,reasons=labels,firstRejectSequence=u(b+24) or None,
             firstRejectChannel=words[b+26] if u(b+24) else None,
             firstRejectValue=words[b+27] if u(b+24) else None,
             statisticalGatePassed=bool(words[b+28] and not reject),operationalBiasImported=False)
    if cal['eligible']!=((words[50]&384)==384):raise ValueError('Calibration evidence contradiction')
    def timing(offset):
        value=u(h+offset)
        return dict(ticks=value if n and value else None,us=value/120 if n and value and not host_only else None,
                    kind='HOST_EVENT_MODEL_NOT_PERFORMANCE' if host_only else 'TIMER0_NOMINAL_120MHZ')
    result['formalBiasFirstReject']=dict(reason=words[52],channel=words[53] if words[52] else None,raw=words[54] if words[52] else None,sequence=u(56) or None)
    result.update(statisticsSamples=n,statisticsFrozen=bool(words[h+5]),
        statistics=totals,segments=segments,rawWindows=window,firstRawAnomaly=anomaly,calibration=cal,
        qepDiagnostics=dict(observedQFLG=f'0x{words[h+28]:04X}',observedQEPSTS=f'0x{words[h+29]:04X}',
            QDFObservedTransitions=words[h+31],newlyObservedStickyBits=f'0x{words[h+32]:04X}',
            limitation='Sticky bit observations are not event counts or encoder qualification.'),
        diagnosticTiming={k:timing(v) for k,v in [('statisticsMax',34),('normalCMax',36),('snapshotCMax',38),('faultCMax',40),('finalC',42)]})
    result['fullFeedbackCalibrationSuite']=result['acquisitionBatchComplete'] and words[8]==6 and n==4096
    result['limitations']+=['Candidate offsets do not authorize gain/polarity or import an approved bias.',
                            'Full-frame statistics preserve raw variation; only 128 consecutive raw samples retained.']
    return result


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('input',type=Path)
    ap.add_argument('--out',type=Path,required=True);ap.add_argument('--host-only',action='store_true');a=ap.parse_args()
    m=load_memory(a.input)
    if set(m)!=set(range(0xC000,0xD000)):raise ValueError('Exact C000-CFFF word-addressed export required')
    w=[m[i] for i in range(0xC000,0xD000)];r=decode(w,a.host_only)
    r['inputSHA256']=hashlib.sha256(a.input.read_bytes()).hexdigest().upper()
    a.out.mkdir(parents=True,exist_ok=False)
    (a.out/'raw_words.json').write_text(json.dumps(w),encoding='utf-8')
    (a.out/'summary.json').write_text(json.dumps(r,indent=2,allow_nan=False),encoding='utf-8')
    for name,key in [('statistics','statistics'),('records','records'),('raw_windows','rawWindows')]:
        rows=r.get(key,[])
        if key=='statistics':rows=[dict(channel=ch,**row) for ch,row in zip(['IU','IV','IW','VBUS'],rows)]
        if not rows:continue
        with (a.out/(name+'.csv')).open('w',newline='',encoding='utf-8-sig') as f:
            writer=csv.DictWriter(f,fieldnames=list(rows[0]));writer.writeheader()
            for row in rows:writer.writerow({k:'N/A' if v is None else json.dumps(v) if isinstance(v,(dict,list)) else v for k,v in row.items()})
    if 'segments' in r:
        with (a.out/'segments.csv').open('w',newline='',encoding='utf-8-sig') as f:
            wr=csv.DictWriter(f,fieldnames=['segment','channel']+list(r['segments'][0][0]));wr.writeheader()
            for k,seg in enumerate(r['segments']):
                for j,row in enumerate(seg):wr.writerow(dict(segment=k,channel=['IU','IV','IW','VBUS'][j],**row))
    lines=['# Stage6回传自动分析','',f"{r['build']}/layout{r['layout']}；{r['evidence']}",
           f"status={r['status']}；processed={r['processed']}；FOC成功/调用={r['focSuccess']}/{r['focCalls']}；faults={r['errorBits']}",
           '物理PWM执行器未编入，真实闭环/完整ISR/WCET均未由此证明。','']
    if 'calibration' in r:
        lines+=['偏置候选：'+str(r['calibration']['candidate']),
                '拒绝原因：'+', '.join(r['calibration']['reasons']),
                '正式审核偏置：未授予；候选不自动导入。','',
                '|通道|完整样本|有效非饱和样本|均值|峰峰值|标准差|','|---|---:|---:|---:|---:|---:|']
        for ch,t in zip(['IU','IV','IW','VBUS'],r['statistics']):
            lines.append(f"|{ch}|{t['count']}|{t['validCount']}|{t['mean']}|{t['pp']}|{t['sd']}|")
    lines+=['','HOST_ONLY时间为事件模型，µs为N/A；真实时间按标称120MHz换算，仍须独立时钟证据。']
    (a.out/'report.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(r['build'],'status',r['status'],'samples',r.get('statisticsSamples','R01 snapshots only'))


if __name__=='__main__':main()
