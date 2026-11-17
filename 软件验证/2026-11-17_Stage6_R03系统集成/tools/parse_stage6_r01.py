"""Stage6 build 26111608 layout 1. Offline, Python 3.9 stdlib only."""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import math
import struct
from ccs_memory import load_memory

BUILD = 0x26111608
FLOATS = ('theta_sample theta_apply omega_mech omega_elec i_alpha i_beta id iq '
          'vd vq v_alpha v_beta modulation_u modulation_v modulation_w '
          'duty_u duty_v duty_w ui_d ui_q vmax qmax current_sum advance_rad').split()


def decode(words, host_only=False):
    if len(words) != 4096:
        raise ValueError('Expected exactly 4096 16-bit words')
    def u(i):
        return words[i] | words[i+1] << 16
    def f(i):
        value = struct.unpack('<f', struct.pack('<I', u(i)))[0]
        if not math.isfinite(value):
            raise ValueError('Nonfinite exported float at word '+str(i))
        return value
    if (u(0), u(2), words[4], words[5]) != (0x53364631, BUILD, 1, 4096):
        raise ValueError('Identity/layout mismatch; do not guess addresses')
    if words[64:67] != [128, 16, 0] or words[14] or words[15] != 1:
        raise ValueError('Layout, source or permanent output lock mismatch')
    if words[6] not in range(5) or words[11] > 16 or words[10] > 1:
        raise ValueError('Invalid header state/count')
    if u(20) > u(18) or u(18) > u(16):
        raise ValueError('Impossible execution counters')
    if words[51] not in (0, 1) or (words[51] and not u(22)):
        raise ValueError('First fault validity mismatch')
    if words[6] == 2 and (u(22) or not words[10]):
        raise ValueError('Done status conflicts with faults/finalization')
    if words[6] == 3 and (not u(22) or not words[10]):
        raise ValueError('Fault status conflicts with faults/finalization')
    if words[11] != min(u(16)//256, 16):
        raise ValueError('Snapshot coverage does not match processed count')
    if words[6] == 2 and u(90):
        if u(26) >= 6000 or u(28) >= 6000 or u(38) or u(40):
            raise ValueError('Done status conflicts with timing/overflow fault evidence')
        if u(68) and u(30) >= 6000:
            raise ValueError('RAM-stage deadline violation cannot pass')
        if u(36) and not (5400 <= u(32) <= u(34) <= 6600):
            raise ValueError('Entry interval violation cannot pass')
    if words[48] and (not words[50] & 1024) and words[49] != 2048:
        raise ValueError('Unreviewed bias is not a full 2048-sample calibration')

    records = []
    def record(base, kind):
        seq = u(base)
        if seq < 1 or seq > u(16):
            raise ValueError('Record sequence outside processed range')
        called, converted, staged = words[base+43:base+46]
        if any(v not in (0, 1) for v in (called, converted, staged)):
            raise ValueError('Invalid record validity flag')
        if staged and (not called or not converted):
            raise ValueError('PWM RAM staging without computed feedback')
        if any(v > 3000 for v in words[base+46:base+49]):
            raise ValueError('RAM compare outside period')
        if words[base+115]:
            raise ValueError('Power-permitted output in locked firmware')
        sf = None
        if called:
            sf = dict(zip(FLOATS, [f(base+64+2*j) for j in range(24)]))
            sf.update(zip(('valid', 'angle_qualified', 'current_qualified',
                           'power_permitted', 'fault_latch'), words[base+112:base+117]))
        elif any(words[base+64:base+118]):
            raise ValueError('Old algorithm output present while called=0')
        item = dict(kind=kind, sequence=seq, entry_tick=u(base+2),
                    sample_estimate_tick=u(base+4), qep_tick=u(base+6),
                    release_estimate_tick=u(base+8), position=u(base+10),
                    control_step_ticks=u(base+12), c_ticks=u(base+14),
                    raw=words[base+16:base+20], adc_flags=words[base+20:base+23],
                    entry_overflow=words[base+23:base+26],
                    pre_clear_overflow=words[base+26:base+29],
                    post_overflow=words[base+29:base+32], tz=words[base+32:base+35],
                    qep_status=words[base+35], qep_flags=words[base+36],
                    gpio=words[base+37], sd=words[base+38], trip=words[base+39],
                    protection_latch=words[base+40], state=words[base+41],
                    blockers=words[base+42], called=called, conversion_valid=converted,
                    pwm_staged=staged, compare=words[base+46:base+49],
                    current_A=[f(base+52+2*j) for j in range(3)] if converted else None,
                    bus_V=f(base+58) if converted else None,
                    iq_reference=f(base+60), speed_reference=f(base+62), output=sf)
        records.append(item)
        return item
    for j in range(words[11]):
        rec = record(384+128*j, 'snapshot')
        if rec['sequence'] != (j+1)*256:
            raise ValueError('Snapshot ownership/order mismatch')
    if words[51]:
        rec = record(128, 'first_fault')
        if rec['sequence'] != u(24):
            raise ValueError('First fault ordinal mismatch')
    if u(16):
        rec = record(256, 'last')
        if rec['sequence'] != u(16):
            raise ValueError('Terminal ownership mismatch')

    full = (words[6] == 2 and words[10] == 1 and words[9] == 1 and
            u(16) == 4096 and words[11] == 16 and not u(22) and words[12] == 0)
    count = u(90)
    def timing(value, measured):
        return dict(ticks=value if measured else None,
                    us=value/120.0 if measured and not host_only else None,
                    kind='HOST_EVENT_MODEL_NOT_PERFORMANCE' if host_only else 'TIMER0_NOMINAL_120MHZ')
    result = dict(build=f'0x{BUILD:08X}', layout=1,
                  evidence='HOST_ONLY' if host_only else 'RAM_EXPORT_LOAD_EVIDENCE_REQUIRED',
                  status=words[6], state=words[7], command=words[8], run=words[9],
                  finalized=words[10], snapshots=words[11], rejected=words[12],
                  blockers=words[13], powerEnable=words[14], actuationCompiled=words[66],
                  processed=u(16), focCalls=u(18), focSuccess=u(20), errorBits=u(22),
                  firstSequence=u(24) if u(70) else None, firstFault=u(70),
                  firstSnapshotAvailable=bool(words[51]),
                  acquisitionBatchComplete=full,
                  fullFeedbackCalculationBatch=full and u(18) == 4095 and u(20) == 4095,
                  realCurrentCalibrationVerified=False, physicalClosedLoopVerified=False,
                  strictISRorWCETVerified=False,
                  outputStagingCount=u(68), biasValid=bool(words[48]), biasSamples=words[49],
                  profileEvidence=words[50],
                  biasOffsets=[f(72+2*j) for j in range(3)] if words[48] else None,
                  biasRawMin=words[78:81], biasRawMax=words[81:84],
                  biasRawSum=[u(84+2*j) for j in range(3)],
                  overflowAffectedIRQs=u(38), deadlineViolations=u(40),
                  timing=dict(controlStepMax=timing(u(92), count > 0),
                              cMax=timing(u(26), count > 0),
                              wrapperMax=timing(u(28), count > 0),
                              estimatedReleaseToRAMStageMax=timing(u(30), u(68) > 0),
                              entryIntervalMin=timing(u(32), u(36) > 0),
                              entryIntervalMax=timing(u(34), u(36) > 0)),
                  records=records,
                  limitations=['RAM snapshot is not independent loading/clock evidence.',
                               'controlStep includes checks/conversion/FOC/speed/staging; not SF_step alone.',
                               'Release is sequential-register estimated SOCA, not latched ADC S/H.',
                               'Wrapper excludes hardware entry/prologue and tail after timestamp/IRET.',
                               'No hardware compare write, physical output or motor operation.'])
    return result


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('input', type=Path)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--host-only', action='store_true', help='Synthetic fixtures only')
    args = ap.parse_args()
    memory = load_memory(args.input)
    if set(memory) != set(range(0xC000, 0xD000)):
        raise ValueError('Export must cover exactly 0xC000-0xCFFF, 4096 16-bit words')
    words = [memory[x] for x in range(0xC000, 0xD000)]
    result = decode(words, args.host_only)
    result['inputSHA256'] = hashlib.sha256(args.input.read_bytes()).hexdigest().upper()
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'raw_words.json').write_text(json.dumps(words), encoding='utf-8')
    (args.out/'summary.json').write_text(json.dumps(result, indent=2, allow_nan=False), encoding='utf-8')
    with (args.out/'records.csv').open('w', newline='', encoding='utf-8-sig') as stream:
        writer = csv.DictWriter(stream, fieldnames=list(result['records'][0]) if result['records'] else ['kind'])
        writer.writeheader()
        for rec in result['records']:
            writer.writerow({k: json.dumps(v, allow_nan=False) if isinstance(v, (list, dict)) else
                             ('N/A' if v is None else v) for k, v in rec.items()})
    lines = ['# Stage6 离线解析', '', '证据类别：'+result['evidence'],
             f"状态 {result['status']}；采集 {result['processed']}/4096；FOC {result['focSuccess']}/{result['focCalls']}",
             f"errorBits={result['errorBits']}；blockers={result['blockers']}；功率输出=0", '',
             '无资格的电流/母线显示 N/A；无样本计时为 null/N/A。主机时间模型不是芯片耗时。', '']
    for name, value in result['timing'].items():
        lines.append(name+': '+('N/A' if value['ticks'] is None else
                               str(value['ticks'])+' ticks / '+
                               ('N/A (HOST event model)' if args.host_only else str(value['us'])+' us')))
    lines.extend(['', '本报告不能证明真实电流校准、完整 ISR/WCET 或真实电机闭环通过。'])
    (args.out/'report.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
    print(json.dumps({k: v for k, v in result.items() if k != 'records'}, indent=2, allow_nan=False))


if __name__ == '__main__':
    main()
