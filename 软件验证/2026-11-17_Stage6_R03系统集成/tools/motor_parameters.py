"""Versioned PMSM tuning proposals; no automatic hardware/run authority."""
from pathlib import Path
import argparse,json,math,hashlib
BUILD=0x26111702

def calculate(p):
    if p['schema']!=3 or int(p['build'],0)!=BUILD:raise ValueError('Wrong schema/build')
    keys=['Rs','Ld','Lq','flux','J','friction','rated_current','max_speed','current_bandwidth_Hz','speed_bandwidth_Hz','damping','speed_slew_rad_s2','iq_slew_A_s']
    for k in keys:
        v=p[k]
        if isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v):raise ValueError('Missing/nonfinite '+k)
    if not (0<p['Rs']<=100 and 1e-6<p['Ld']<1 and 1e-6<p['Lq']<1 and 0<p['flux']<10 and 0<p['J']<10 and 0<=p['friction']<10):raise ValueError('Motor model range')
    if not (0<p['rated_current']<=2.5 and 0<p['max_speed']<=1000):raise ValueError('Software development range; not a motor nameplate')
    if not (0<p['current_bandwidth_Hz']<=1000 and 0<p['speed_bandwidth_Hz']<=50 and p['speed_bandwidth_Hz']<=p['current_bandwidth_Hz']/10 and .5<=p['damping']<=2):raise ValueError('Bandwidth separation / Nyquist reserve')
    if type(p['counts_per_rev']) is not int or not 4<=p['counts_per_rev']<=16777216 or type(p['pole_pairs']) is not int or not 1<=p['pole_pairs']<=100 or p['direction'] not in [-1,1]:raise ValueError('Encoder parameters')
    if not (0<p['speed_slew_rad_s2']<=10000 and 0<p['iq_slew_A_s']<=10000):raise ValueError('Reference slew limits')
    wi=2*math.pi*p['current_bandwidth_Hz'];ws=2*math.pi*p['speed_bandwidth_Hz'];kt=1.5*p['pole_pairs']*p['flux']
    gains=dict(kp_d_V_per_A=p['Ld']*wi,kp_q_V_per_A=p['Lq']*wi,ki_d_series=p['Rs']/p['Ld']/20000,ki_q_series=p['Rs']/p['Lq']/20000,speed_kp_A_per_rad_s=2*p['damping']*ws*p['J']/kt,speed_ki_A_per_rad=ws*ws*p['J']/kt)
    if any(not math.isfinite(v) for v in gains.values()) or max(gains['kp_d_V_per_A'],gains['kp_q_V_per_A'])>100 or max(gains['ki_d_series'],gains['ki_q_series'])>1 or gains['speed_kp_A_per_rad_s']>10 or gains['speed_ki_A_per_rad']>100:raise ValueError('SF/controller implementation range')
    return dict(gains=gains,Ts_current_s=.00005,Ts_speed_s=.001,compatibleWithRetainedSymmetricSF=p['Ld']==p['Lq'],evidenceGranted=False,reviewStatus=p['review_status'],voltageReserve='vmax=min(reviewed modulation fraction,1/sqrt(3))*measured calibrated bus; saturation is dynamic',model='phase dq SI; Kp=L*wc; series Ki=R/L*Ts; speed torque constant=1.5*p*flux; friction not subtracted from Kp (conservative model mismatch)',limitations=['Nominal model tuning, no plant stability proof.','Ld != Lq proposals cannot import into retained single-gain SF.','No power, encoder zero or sensor evidence granted.'])

def validate_review(p,root):
    if p['review_status']!='APPROVED':raise ValueError('Physical model still PENDING')
    calculate(p)
    if p['Ld']!=p['Lq']:raise ValueError('Retained SF has shared d/q gains; asymmetric model requires separate reviewed extension')
    if not p.get('reviewer') or not p.get('conditions') or not p.get('raw_sources'):raise ValueError('Missing reviewed provenance')
    if p.get('parameter_convention')!='PHASE_DQ_SI':raise ValueError('Line-line parameters may not import as phase dq')
    for item in p['raw_sources']:
        f=(root/item['path']).resolve()
        if not f.is_relative_to(root.resolve()) or not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest().upper()!=item['sha256'].upper():raise ValueError('Evidence path/hash mismatch')
    return True

def generate(p):
    data=calculate(p)
    if not data['compatibleWithRetainedSymmetricSF']:raise ValueError('Asymmetric SF unsupported')
    mapping=dict(RS='Rs',LD='Ld',LQ='Lq',FLUX='flux',J='J',B='friction',CURRENT='rated_current',SPEED='max_speed',CURRENT_HZ='current_bandwidth_Hz',SPEED_HZ='speed_bandwidth_Hz',DAMPING='damping',SPEED_SLEW='speed_slew_rad_s2',IQ_SLEW='iq_slew_A_s')
    lines=['#ifndef S6_MOTOR_PROFILE_H','#define S6_MOTOR_PROFILE_H','#define S6_MOTOR_REVIEWED 1U','#define S6_MOTOR_BUILD 0x26111702UL']
    for macro,key in mapping.items():
        value=format(p[key],'.9g');value=value if '.' in value or 'e' in value else value+'.0';lines.append(f'#define S6_MOTOR_{macro} {value}f')
    lines.extend([f"#define S6_MOTOR_CPR {p['counts_per_rev']}UL",f"#define S6_MOTOR_POLES {p['pole_pairs']}U",f"#define S6_MOTOR_DIRECTION {p['direction']}",'#endif',''])
    return '\n'.join(lines)

def main():
    ap=argparse.ArgumentParser();ap.add_argument('profile',type=Path);ap.add_argument('--out',type=Path,required=True);ap.add_argument('--evidence-root',type=Path);ap.add_argument('--emit-header',type=Path);a=ap.parse_args()
    p=json.loads(a.profile.read_text(encoding='utf-8-sig'));result=calculate(p)
    if a.emit_header:
        if a.evidence_root is None:raise ValueError('Evidence root required')
        validate_review(p,a.evidence_root)
        if a.emit_header.exists():raise ValueError('Do not overwrite frozen header')
        a.emit_header.write_text(generate(p),encoding='utf-8')
    if a.out.exists():raise ValueError('Do not overwrite results')
    a.out.write_text(json.dumps(result,ensure_ascii=False,indent=2,allow_nan=False),encoding='utf-8');print('PARAMETER_PROPOSAL_ONLY; no run authority')
if __name__=='__main__':main()
