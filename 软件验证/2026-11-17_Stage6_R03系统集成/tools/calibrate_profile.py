"""Offline physical calibration fits and reviewed-profile validation.
Fits never grant evidence. CSV reference values must be synchronized and powered.
"""
from pathlib import Path
import argparse,csv,hashlib,json,math


def fit(points,kind):
    if kind not in ('bus','IU','IV','IW'):raise ValueError('Unknown channel')
    if len(points)<4:raise ValueError('At least 3 fit points and 1 independent holdout required')
    for p in points:
        if p['role'] not in ('fit','holdout') or p['powered']!='yes' or p['synchronous']!='yes':
            raise ValueError('Every point needs active sampling supply and synchronized reference')
        if len(p['raw_sha256'])!=64 or any(x not in '0123456789abcdefABCDEF' for x in p['raw_sha256']):
            raise ValueError('Point raw provenance SHA256 missing')
        if not p['instrument'].strip():raise ValueError('Reference instrument missing')
        for name in ['raw','reference']:
            if not math.isfinite(float(p[name])):raise ValueError('Nonfinite calibration value')
        if not 1<float(p['raw'])<4094:raise ValueError('ADC rail or invalid raw')
    train=[p for p in points if p['role']=='fit'];test=[p for p in points if p['role']=='holdout']
    if len(train)<3 or not test:raise ValueError('Need independent holdout')
    x=[float(p['raw']) for p in train];y=[float(p['reference']) for p in train]
    if len(set(x))<3:raise ValueError('Insufficient distinct raw points')
    if kind!='bus' and not (min(y)<0<max(y)):raise ValueError('Current polarity fit requires both signed directions')
    if kind=='bus' and min(y)<0:raise ValueError('Negative bus reference')
    mx=sum(x)/len(x);my=sum(y)/len(y);den=sum((v-mx)**2 for v in x)
    if den<=0:raise ValueError('Degenerate fit')
    gain=sum((a-mx)*(b-my) for a,b in zip(x,y))/den;intercept=my-gain*mx
    if not math.isfinite(gain) or abs(gain)<1e-9 or abs(gain)>=1 or kind=='bus' and gain<=0:
        raise ValueError('Invalid affine gain')
    zero=-intercept/gain
    if not -4096<=zero<=4096:raise ValueError('Implausible affine zero')
    residuals=[dict(role=p['role'],raw=float(p['raw']),reference=float(p['reference']),
                    predicted=(float(p['raw'])-zero)*gain,
                    error=(float(p['raw'])-zero)*gain-float(p['reference'])) for p in points]
    return dict(channel=kind,model='physical=(raw-zero_count)*signed_gain',zero_count=zero,
                signed_gain=gain,fitRMSE=math.sqrt(sum(r['error']**2 for r in residuals if r['role']=='fit')/len(train)),
                holdoutMaxAbsError=max(abs(r['error']) for r in residuals if r['role']=='holdout'),
                fitRawSpan=max(x)-min(x),referenceSpan=max(y)-min(y),residuals=residuals,
                evidenceGranted=False,reviewStatus='PENDING',provenanceSHAStatus='FORMAT_ONLY_NOT_FILE_VERIFIED',
                limitation='Fit uncertainty requires instrument and full-sample noise/drift budgets; no automatic approval.')


def validate(profile,root):
    if profile['schema']!=2 or profile['hardware']!='EBF410194V1R2+LAUNCHXL-F280039C':raise ValueError('Unknown profile schema/hardware')
    allowed=0
    for ch in ['IU','IV','IW','VBUS']:
        item=profile['channels'][ch]
        if item['review_status'] not in ['PENDING','APPROVED']:raise ValueError('Invalid review status')
        if item['review_status']=='PENDING':continue
        if not item.get('reviewer') or not item.get('experiment') or not item.get('instrument') or not item.get('conditions'):
            raise ValueError('Approved coefficient lacks review record')
        for evidence in item['raw_sources']:
            path=(root/evidence['path']).resolve()
            if not path.is_relative_to(root.resolve()) or not path.is_file():raise ValueError('Missing local evidence file')
            if hashlib.sha256(path.read_bytes()).hexdigest().upper()!=evidence['sha256'].upper():raise ValueError('Raw evidence hash mismatch')
        if not item['raw_sources']:raise ValueError('No raw sources')
        gain=item['gain'];zero=item['zero_count'];limit=item['verified_range']
        if not all(isinstance(x,(int,float)) and math.isfinite(x) for x in [gain,zero,limit]):raise ValueError('Illegal coefficient')
        if not 0<abs(gain)<1 or not -4096<=zero<=4096 or limit<=0:raise ValueError('Coefficient/range invalid')
        if ch!='VBUS' and not 1536<=zero<=2560:raise ValueError('Current zero outside retained calibration bounds')
        if ch=='VBUS' and gain<=0:raise ValueError('Bus slope must be positive')
        if not item.get('holdout_verified') or not item.get('gain_polarity_verified'):raise ValueError('Independent gain/polarity validation missing')
    if all(profile['channels'][ch]['review_status']=='APPROVED' for ch in ['IU','IV','IW']):allowed|=1|1024
    if profile['channels']['VBUS']['review_status']=='APPROVED':allowed|=2
    # Interface and zero-current authorization remain separate reviewed records.
    for field,bit in [('interface_review',128),('zero_current_review',256)]:
        item=profile[field]
        if item['status']=='APPROVED':
            if not item.get('reviewer') or not item.get('record'):raise ValueError('Missing condition review record')
            record=(root/item['record']).resolve()
            if not record.is_relative_to(root.resolve()) or not record.is_file():raise ValueError('Condition evidence missing')
            if hashlib.sha256(record.read_bytes()).hexdigest().upper()!=item.get('sha256','').upper():raise ValueError('Condition evidence hash mismatch')
            allowed|=bit
        elif item['status']!='PENDING':raise ValueError('Unknown condition status')
    return allowed


def header(profile,evidence):
    lines=['/* Generated by offline review tool; never WATCH input. */','#ifndef S6_CALIBRATION_PROFILE_H',
           '#define S6_CALIBRATION_PROFILE_H',f'#define S6_PROFILE_EVIDENCE {evidence}U']
    for name in ['IU','IV','IW','VBUS']:
        p=profile['channels'][name];approved=p['review_status']=='APPROVED'
        for field,suffix in [('gain','GAIN'),('zero_count','ZERO'),('verified_range','RANGE')]:
            value=p[field] if approved else 0.0
            text=format(value,'.9g');text=text if '.' in text or 'e' in text.lower() else text+'.0'
            lines.append(f'#define S6_PROFILE_{name}_{suffix} {text}f')
    lines+=['#endif',''];return '\n'.join(lines)


def main():
    ap=argparse.ArgumentParser(description=__doc__);sub=ap.add_subparsers(dest='cmd',required=True)
    f=sub.add_parser('fit');f.add_argument('csv',type=Path);f.add_argument('--channel',required=True);f.add_argument('--out',type=Path,required=True)
    v=sub.add_parser('validate');v.add_argument('profile',type=Path);v.add_argument('--evidence-root',type=Path,required=True);v.add_argument('--header',type=Path)
    a=ap.parse_args()
    if a.cmd=='fit':
        result=fit(list(csv.DictReader(a.csv.open(encoding='utf-8-sig'))),a.channel)
        if a.out.exists():raise ValueError('Do not overwrite calibration result')
        a.out.write_text(json.dumps(result,indent=2,allow_nan=False),encoding='utf-8');print('FIT_ONLY: review pending')
    else:
        p=json.loads(a.profile.read_text(encoding='utf-8'));e=validate(p,a.evidence_root)
        if a.header:
            if a.header.exists():raise ValueError('Do not overwrite reviewed header')
            a.header.write_text(header(p,e),encoding='utf-8')
        print('Validated offline profile evidence mask:',e,'; power remains permanently locked')


if __name__=='__main__':main()
