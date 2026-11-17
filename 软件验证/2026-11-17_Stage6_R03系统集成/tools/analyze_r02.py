"""Offline R02 real evidence correlation; never infer full 4096-waveforms."""
from pathlib import Path
import json,csv,math
R=Path(__file__).resolve().parents[3];H=Path(__file__).resolve().parents[1]
def analyze(evidence,out):
    r=json.loads((evidence/'解析结果/summary.json').read_text(encoding='utf-8-sig'))
    labels=['IU','IV','IW','VBUS'];windows=r['rawWindows'];corr=[];auto=[]
    def coefficient(x,y):
        mx=sum(x)/len(x);my=sum(y)/len(y);den=math.sqrt(sum((v-mx)**2 for v in x)*sum((v-my)**2 for v in y))
        return sum((a-mx)*(b-my) for a,b in zip(x,y))/den if den else None
    for kind in ['startup','tail']:
        rows=[w for w in windows if w['window']==kind];assert len(rows)==64
        for i in range(4):
            x=[w['raw'][i] for w in rows]
            auto.append(dict(window=kind,channel=labels[i],lag1=coefficient(x[:-1],x[1:]),mean=sum(x)/64,pp=max(x)-min(x)))
            for j in range(i+1,4):corr.append(dict(window=kind,a=labels[i],b=labels[j],pearson=coefficient(x,[w['raw'][j] for w in rows])))
    segments=[]
    for j,ch in enumerate(labels):
        means=[s[j]['mean'] for s in r['segments']];sds=[s[j]['sd'] for s in r['segments']]
        segments.append(dict(channel=ch,meanRange=max(means)-min(means),sdMin=min(sds),sdMax=max(sds),SD=r['statistics'][j]['sd'],PP=r['statistics'][j]['pp']))
    out.mkdir(parents=True,exist_ok=False)
    for name,rows in [('correlations',corr),('lag1',auto),('segments_summary',segments)]:
        with (out/(name+'.csv')).open('w',encoding='utf-8-sig',newline='') as f:
            wr=csv.DictWriter(f,fieldnames=list(rows[0]));wr.writeheader();wr.writerows(rows)
    result=dict(evidence='REAL_R02_RETAINED_WINDOWS_ONLY',rawSHA256=r['inputSHA256'],correlations=corr,lag1=auto,segments=segments,limitation='Two disjoint 64-point windows; not a 128-point continuous trace, full spectrum, common-ground proof or root-cause identification.')
    (out/'analysis.json').write_text(json.dumps(result,indent=2),encoding='utf-8');return result
if __name__=='__main__':
    a=analyze(R/'实物记录/2026-11-17/Stage6_R02_CAL_01/回传原件',H/'results/R02_correlations');print(json.dumps(a,indent=2))
