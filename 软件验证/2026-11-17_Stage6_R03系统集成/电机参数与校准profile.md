# 模型参数、量纲及受控导入

profile/motor_profile.json为实际42JSF840AS-1000-8档案，review_status=PENDING；Rs、Ld/Lq、磁链、J、额定电流、实际速度及方向等未知。CPR4000/极对数4是例程参考，不自动授证。.59Ω/.66mH尚未厘清单相/线线，不作为运行系数。
profile/HOST_ONLY_motor.json是明确的合成模型：.59Ω/.66mH、磁链.03Wb、J.00002kg·m²、1A、500Hz电流/10Hz速度带宽等，不属于真实电机测量。

Ts电流50µs；Ts速度1ms。相dq SI下，Kp_d=Ld·2πfci、Kp_q=Lq·2πfci，单位V/A；原series PI的Ki=(R/L)Ts，无量纲。转矩常数Kt=1.5·p·磁链，速度Kp=2ζωnJ/Kt，Ki=ωn²J/Kt；速度积分使用实际累积dt。摩擦保留模型但不从Kp扣除，属简化整定。
原SF只提供共同d/q增益；计算工具可显示Ld≠Lq分别建议，但导入拒绝不对称模型，不能假装两轴独立整定已支持。默认电流带宽≤1kHz、速度≤50Hz且≤电流/10，为软件整定范围，不是实物稳定性承诺。
原SF按测量母线动态限幅vmax≤Vbus/√3，dq电压圆约束和积分限幅保留；整定工具不证明给定转速下反电势/电阻压降、延迟和电流动态的母线余量。额定转速/磁链确认后需联合评估ωe·磁链、R·I、Ldi/dt及调制保留。

示例离线命令（新输出目录，禁止覆盖冻结文件）：
`python 解析工具/motor_parameters.py 校准/profile/HOST_ONLY_motor.json --out HOST_ONLY_tuning.json`
审核实际参数时，schema3/build26111702、PHASE_DQ_SI、来源路径/SHA、仪器/条件/审核者必须完整；--emit-header还要求--evidence-root并通过审查。生成头只授控制模型证据，永远不授操作者/PWM/保护/磁零位权限。
实际电流/IU-IV-IW各自有符号A/count、zero/range和母线仿射profile沿用R02的受控工具，旧校准文件另列“历史R02参考”；实际证据仍PENDING。不能以脚本合成APPROVED夹具作为真实资料。

电流独立校准需经审核低能量回路通过目标分流器，正负已知电流及独立留出点；不直接24V跨接分流或电机。母线至少3个有效供电点训练+1留出，V=(raw-zero)·gain；断电失供RAW低码不能作0V点。具体注流/电压范围和接线需单独授权，不因本包工具存在而放行。
