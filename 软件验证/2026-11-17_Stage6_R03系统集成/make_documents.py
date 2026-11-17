"""Write R03 review material from the audited final image; no target actions."""
from pathlib import Path
import json

H=Path(__file__).resolve().parent
R=H.parents[1]
I=json.loads((H/'audit/image_identity.json').read_text())
RES=json.loads((H/'audit/resources.json').read_text())
SHA=I['OUT_SHA256']; READY=I['S6_readyStop']['address']; DONE=I['S6_doneStop']['address']
HEADER=f'''工程 Stage6_SENSORED_FOC；配置 CPU1_SENSORED_FOC_RAM；build **0x26111702**；layout **3**。
OUT SHA256：`{SHA}`。
初始化 `{READY}`，完成 `{DONE}`；均为 `7700 FF69 0006`（NOP/SPM #0/LRETR）。
命令 gStage6Command=`0x0400`，结果 gStage6=`0xC000–0xCFFF`，4096个16位字。
'''

DOCS={}
DOCS['00_使用说明.md']=f'''# Stage6 R03 系统集成冻结交付

{HEADER}
本地状态：**LOCAL_SOFTWARE_AND_BUILD_VERIFIED**。本包没有R03芯片实测，不是实际电机运行放行包。

已完成实际源码：真实ADC/eQEP输入与原EF/SF电流环、速度外环集成；三相PWM事务、运行生命周期、独立资格及有界操作者授权、受控停止、首错锁存；电机参数工具；真实目标PWM后端独立对象编译。
可加载OUT仅使用LOCKED后端，全程禁能。真实CMPA/global-load后端位于target_review_only，构建排除且最终MAP/符号无该对象/入口。无清OST、GPIO功率复用或解锁提供者。

先读《结果与结论》《01_控制架构与源码审计》《03_综合示波器测量操作单》；《02_CCS身份与集中导出》仅保存实际身份和后续验收合同，本轮不自动安排加载或新CAL实验。
旧RAW/CAL与Stage5专项关闭迭代，原能力/证据保留；不再开展相同CAL_02，不重测IU，不并行SMO。

包内包含正式工程源码/配置、冻结OUT/MAP/反汇编/链接信息、独立后端对象、完整diff、最终O0/O2日志及数据、原R01/R02真实原件、三代解析器、实际PENDING及HOST_ONLY参数、校准与参数工具、审计和SHA256SUMS。
先执行 `python 校验包.py`，再执行 `python 运行包内自检.py`；两者仅离线读取，测试输出写临时目录。主机C回归重建入口见《主机验证与负例结果》。
只有另获用户授权且满足硬件前置条件，才可实施示波器测量、物理校准、PWM/保护独立验证或电机动作。本轮未进行任何连接、加载、写目标RAM、上电或试转。
'''

DOCS['结果与结论.md']=f'''# R03 软件集成及真实R02复核结论

{HEADER}
已完成软件集成和本地验证；**LOCAL_SOFTWARE_AND_BUILD_VERIFIED**。
正式控制算法已接到ADC/eQEP端口和受资格管理的RAM PWM事务。真实三相影子寄存器与联动装载源代码已由TI单独编译，隔离于本次OUT。默认真实参数与资格仍PENDING，因此不是可直接解锁运行的电机固件。

## 已完成与证据

- GCC O0/O2严格浮点及Wall/Wextra/Werror构建和运行通过，33份DAT/CSV逐字节一致；包含4096拍保留算法对照、连续影子运行、完整主机电流/速度闭环及故障负例。
- 新增64项解析、参数导入、篡改拒绝和R01/R02真实DAT兼容检查通过；旧三份真实结果独立重解析与原JSON完全相同。
- TI CGT25.11.1.LTS、FPU32/EABI/O2/strict/fp_reassoc=off，全量构建0错误0警告。加载映像未采用fast-math、TMU/CLA新路径。
- RAM代码11809/16384字，余4575；控制数据546/4096字；导出4096字。栈分配728字，非嵌套静态路径估算344字，实际栈峰值/NMI未测。
- 10343份受保护历史文件哈希未变；16份数学/端口/板级/保护/链接/CCXML文件逐字节不变；完整diff单列实际变更。

## 真实R02结果

原ZIP27项清单全部匹配，真实DAT SHA256为9499E85FC8095CA3034DEC825AED867E74BB71D8B959DA6C953245A2D9645EA0。
4096拍完整、无应用fault/OVF/deadline记录、FOC0。三相SD6.0667/7.7841/13.5906码，pp49/56/76；候选2048拍span48/44/76，reject11，biasValid0/approved0。原32码门限未变，未导入候选均值。
W在各段都具有较大瞬时波动，均值漂移不到1.5码；保留的两段64点中W与VBUS相关系数−0.361/−0.653。这是联合测供电/参考/模拟节点的线索，不是硬件噪声或ADC内部故障的确证。
R02 wrapper4230ticks/35.25µs、统计2511ticks/20.925µs仅属综合诊断路径，不能与历史SF耗时相加评判R03。

## 尚未通过

真实电流独立增益/极性和零偏、母线仿射精度、编码器审核方向/极对数/磁零位、实际采样窗口和驱动极性、硬件过流阈值/传播、真实PWM提交/装载、R03完整ISR截止时间、真实电机电流/速度闭环均未验证。
物理参数Rs/Ld/Lq/磁链/J等保持PENDING；0.59Ω/0.66mH单相与线线定义未知。HOST_ONLY参数只用于软件分支与简化模型。
历史SFfault8、SMO及Stage5 R01/R02/R03失败全部保留，不改判。

## 下一步

先由用户单独授权一次综合示波器测量，联合比较W源端、接收端和电源/参考，减少重复采集；按实际仪器确认可测范围。随后审核真实反馈校准和下桥采样/PWM波形，再分离验证保护与PWM电气条件，最后才评审有电流约束的磁零位和电机闭环。
当前不需为完成本地开发打开CCS；本轮未安排R03上板或原样CAL重跑。每项物理步骤均另行授权。
'''

DOCS['01_控制架构与源码审计.md']='''# 正式控制架构与实际修改依据

## 控制链

ADC SOCA→A/C SOC0及B SOC0/SOC1→ADCB1 ISR→完成/OVF/保护检查→eQEP与时间估计→审核profile换算三相电流/母线→EF角度/速度→原Clarke/Park→原Id/Iq PI及电压限幅→逆Park/SVGEN→三相PWM准备/截止检查→锁定RAM事务。
速度环每20电流拍更新一次，合计实际计数与ticks计算速度，参考斜率限制、积分预装、限幅/抗积分饱和；新Iq显式在下一电流拍使用。默认20kHz电流/1kHz速度。
不新增SMO，不执行旧motor1CtrlISR。保留数学SF/EF和TI变换/PI/SVGEN逐字节不变。

## 变更文件与具体职责

|文件|实际实现与理由|
|---|---|
|s6_pwm.c/h|三相完整准备后发布；范围/死区最小边界、采样窗判定、seq/session、pending覆盖拒绝、绝对截止检查、首次执行故障、提交/消费计数|
|target_review_only/s6_pwm_target.c、s6_pwm_target.h|真实目标寄存器执行：CMPA影子三相准备、GLDCTL2联动ZERO装载、重新读Timer截止检查、OST安全停止；独立对象编译，不链接正式OUT|
|s6_system.c/h|12态生命周期、七类资格、有限会话与模式授权、运行/锁定谓词选择、模式切换、参考斜率、受控停机、首错不清除|
|s6_alignment.c/h|固定定子坐标电流调节、渐升/有界保持、实际QEP静止检查、仅导出磁零位候选；锁定映像不可进入功率标定态|
|s6_motor.c/h、s6_motor_profile.h|受控电机模型校验及PI参数计算，版本绑定；未审核默认全0，只授控制模型证据，不授反馈/保护/功率权限|
|s6_control.c/h|保留算法的共同stepDomain；区分锁定和未来运行保护谓词，速度上限、执行故障码；保持原采样/FOC公式|
|s6_speed.c|增加模式入口参数完整性检查，避免NaN等拖到慢环执行时才发现；原速度PI计算保持|
|s6_profile.c|profile版本3，电机模型导入与失配拒绝，软件范围和版本校验；S6_powerAllowed恒0|
|s6_app.c/h|集成system/PWM、layout3小头扩展；正式影子路径不运行cmd6完整统计；停止邮箱由ISR单一所有者消费，避免前台修改运行中的浮点参考和状态|
|.cproject|明确排除target_review_only；保留既有编译浮点和链接选项|
|wildfire_lock.h|只更新build身份|

完整清单还包含README更新，见审计/changed_files.json和完整源码差异.diff。s6_port.c、wildfire_board.c、wildfire_lock.c、board_hal、链接文件与全部CCXML不改；ADC参考、ACQPS、SOCA、GPIO、CMPSS/XBAR/TZ/OST配置不改。

## 生命周期及资格

INIT→DISABLED→SELFTEST→READY；独立CALIBRATION/SHADOW；未来POWER_READY→ZERO_ALIGN或CURRENT/SPEED→STOPPING→DISABLED；故障进入FAULT且不提供清故障命令。
反馈1、机械编码器2、控制参数4、PWM/采样窗8、保护16、实时性32、操作者64，功率准备需127且backend非LOCKED。磁零位独立于机械资格，电流/速度功率态还要求ZERO证据与当前build/session一致。对齐需要机械编码器及其他六类条件，不预先要求磁零位，避免循环依赖。
操作者授权单独绑定session、模式和最多40000拍未来有效期；profile或一个WATCH标志不能创建该授权。本固件没有填写硬件操作者授权的入口。
STOP保留当前电流PI，Id/Iq按斜率回零后退出计算；这是撤转矩并滑行，不保证机械速度为零，不是刹车。速度专用授权可完成自身停止，不额外要求电流模式授权。
电流↔速度切换保留电流PI；速度PI以旧Iq和测量速度预装并受斜率约束。当前速度模式限定Id=0，拒绝非零Id或待处理非零Id请求，避免独立Iq上限与矢量上限冲突；本轮不实现弱磁/MTPA。电流模式接受dq参考矢量限幅。
正常停机后自检可重置软件PI/时间prime；不清真实故障、trip、校准证据。物理故障后需另行检查与授权恢复，禁止自动重试。

## 参数与磁零位限制

EF角度预览与相对转动可用于影子算法，不能用相对零点作磁极零位。认证zero绑定原始QEP原点与会话；保留SF初始化合同要求首次认证原点匹配，其他起始位置不自动重绑定，安全拒绝。未来需在正式磁零位实验中明确原点建立和重启策略，再放行不同位置启动。
固定坐标对齐为已实现的有界候选分支，不代表物理转矩方向或电角零位已验证；零偏不会自动导入profile。

## 保护

CMPSS1/3/2分别对应IU/IV/IW，HPMux/LPMux=0→TRIP4汇聚→DCAEVT1→TZ；GPIO8为SD_A。当前DAC高0/低4095是fail-closed占位，Trip7/latch1/TZ=C不能判真实过流。
锁定域要求三相OST且六GPIO低；未来运行域要求TZ无故障、trip/latch为0及审核SD健康电平。两种谓词分开，未用“OST始终置位”宣称已输出。
输入丢失/OVF、时序、母线/电流范围、QEP错误、超速、非法状态、过期授权和执行错误全部锁存首错。运行域只通过HOST_ONLY模拟覆盖，实际保护阈值/硬件响应仍PENDING。
'''

DOCS['PWM执行与采样联合合同.md']='''# PWM执行、采样与应用时刻

## 当前可加载映像

TBPRD3000、上下计数、120MHz、20kHz、死区60ticks、SOCA=PERIOD、ACQPS18全部不改。AQ上计CMPA置高、下计置低，RAM占空比约定compare=round((1-duty)*3000)。实际驱动反相/桥臂对应未实测，不能将数学占空比视为物理上桥导通率。
S6P_stagePWM仍只做范围检查。新增RAM事务会保存三相compare、seq/session/release/apply，下一控制帧消费上一RAM事务；“commits/loads”在LOCKED域仅为RAM提交/消费，不是寄存器装载或真实PWM输出证据。
最小compare边界默认60，拒绝端点及NaN/Inf；这只是死区相容的软件底线，真实最小脉宽/器件限制需重新审核，不能据此放行60tick脉冲。

## 已实现、但隔离的目标执行后端

S6T_configure只允许三相OST已置位、TBCLKSYNC关闭、极性及采样窗已审核。冻结单独CMPA装载，启用CMPA全局装载、ZERO触发、one-shot，ePWM1/2/6的GLDCTL2链接主ePWM1。
S6T_submit检查独立permit、前事务已观察装载、无TZ故障；写完整三相CMPA影子后再次核对截止；只设置一次主OSHTLD来联动三相，不逐相放行装载。调用必须是ISR单一所有者、INTM禁止嵌套，且三TB同步已有独立证据。
异常只置OST并锁存，不清OST、不改GPIO、不打开驱动。实际arming提供者本轮不存在。
S6T_observeLoad观察OSHTLD清除和计划装载时刻，记录观察Timer；不是硬件锁存装载时间，也不能证明驱动输出极性。实际外设联动行为要用波形验收。

## 联合整改候选

若经波形确认A实际驱动上桥且不反相，则A在PERIOD附近高、互补下桥可能低，原PERIOD采样不能自动保证三下桥电流有效。
候选为：审核极性→三TB同步→ZERO附近三下桥有效窗→ZERO SOCA/S&H→ISR/控制→下一ZERO联动提交。目标后端配置中SOCA=ZERO与global-load ZERO成套实现，隔离源文件不会更改本次OUT。
每相必须满足 min(CMPA) > dead + blank + settle + acquire（均按SYSCLK ticks），当前acquire19。blank/settle实际未知；主机30/120ticks只为分支夹具，不是测量结果。
窗口不足则拒绝，不能过滤原始电流假装有效。本轮没有高调制度两相重构；未来若三相窗不能满足，应单独审核有效相选择/重构和相应限调制策略。
guard默认120ticks是待验证的软件预留；deadline要求 release-now < 6000-guard，边界5880被拒绝。必须按实际最后提交到ZERO剩余时间、总线写入和同步延时实测审核guard，不能宣称120ticks足够。

## ADC、QEP和角度

A/C SOC0采电流，B SOC0电流、SOC1母线，ADCB EOC1触发；母线与电流S/H不同时。当前19SYSCLK采样约158.3ns；B两SOC完成约123SYSCLK的手册估计不含全部仲裁/同步，不能当成实测。
原S6_port用TBSTS/TBCTR/Timer顺序读取估算上一PERIOD事件；不是硬件锁存S/H时间，TB方向边界有读序误差，原估计方法保持未改。SF使用ADC估计、实际QEP读取tick及计划应用tick作恒速角度外推。QEP实际读取时刻与首个计时读取也存在指令延迟。
未来ZERO后端尚需一并替换/审核实际release估计和ADC/QEP/应用时刻合同，不能直接把隔离对象加入原PERIOD端口后就运行。本轮只交付源实现与独立编译，不伪称目标端到端联调通过。

## 模拟建立

R108100Ω/C440.1µF设计RC约10µs；全幅阶跃到0.5LSB约90µs是外部前端响应，不是应设的ADC ACQPS。仅以100Ω源、Ron860Ω、Ch7.5pF估算约65ns，小于现有158ns，但不涵盖真实线缆/运放/参考/MUX历史。
控制允许的瞬时误差应由审核A/count、目标电流误差和保护误差预算计算，之后用实际阶跃/波形验证建立时间。当前无充分证据修改ACQPS或SOCA，原基线不变。
'''

DOCS['电机参数与校准profile.md']='''# 模型参数、量纲及受控导入

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
'''

DOCS['真实R02复核与反馈质量.md']='''# 已完成的真实R02 CAL_01离线复核

原ZIP保全，27项原SHA清单通过；DAT9499E85FC8095CA3034DEC825AED867E74BB71D8B959DA6C953245A2D9645EA0。
新解析器与原版均独立读取实际DAT（未用host-only），summary与回传JSON完全匹配。完整数据不是新采样。

|通道|4096拍均值|标准差|原始最小..最大|峰峰|
|---|---:|---:|---|---:|
|IU|2048.197265625|6.066658717|2026..2075|49|
|IV|2043.5859375|7.784069362|2018..2074|56|
|IW|2046.7314453125|13.590629939|2006..2082|76|
|VBUS|2829.6708984375|11.203641148|2793..2860|67|

2048拍候选均值2048.0322265625/2043.6953125/2046.62646484375，span48/44/76；拒绝11=接口未审核1+零电流未审核2+span8。未批准、biasValid0，不导入本次候选。
首原始诊断异常第6拍IW2036→2073跳37；饱和0，跳变>32数量IU/IV/IW/VBUS=0/4/341/139。诊断异常不是应用fault，原始信息全部保留。
八段均值范围IU.668/IV.711/IW1.469/VBUS1.453码；IW各段SD13.30..13.98，支持短时稳定均值上叠加持续瞬时波动，不能排除长时间温漂。
头64点、尾64点是两段不连续窗口，不能拼接为连续128点求完整频谱/全拍协方差。W-VBUS Pearson分别−.361/−.653，IV-VBUS约−.396/−.393；有限样本相关不证明共因/供电方向或ADC故障，亦可能与采样时序/混叠有关。具体CSV和lag1/分段见结果/R02_correlations。

用户确认母线24V条件；记录母线23.996V、24V侧29mA/限流1A、5VIN4.6778V、3V3_IN3.3024V、IU1.6433V/IV1.6433V/IW1.6463V/VBUS2.2815V。这些是发令前记录，并非逐样同步；两张初始化/测量截图已人工查看，其他截图保全，未以推测补齐。
独立实际Load Program日志、结束PC/SP/停止事件/调用栈/断点完整证据、导出格式截图、仪器型号/时间/测量参考地等缺项仍未知。
QEP位置0、OR flags28/statusE2为粘滞观察；不能把OR当新事件次数或资格。TZ=C、GPIO0、Trip7/latch1属既有锁定占位链，不能判真实过流。
真实R02只证明本次反馈采集/统计完成：status2/run1/finalized1/processed4096/snapshots16/fault0/OVF0/deadline0、FOC0、物理输出0。没有正式校准/真实FOC通过。

原32count规则保持；应分别评价偏置均值不确定度、瞬时电流噪声、长期温漂、实际闭环带宽和保护误差。缺真实A/count时不把13.59count换算成已认证电流误差；不以低通后pp冒充原始反馈通过，不从一次短窗均值稳定推导独立同分布或长期稳定。
'''

DOCS['03_综合示波器测量操作单.md']='''# 下一组综合反馈与波形测量草案（须另行授权）

本单不发命令、不要求重跑CAL_02、不要求加载R03。先根据实际仪器及原理图核对测点，用户授权接线/上电后才开展。现有已供电现场若需改探头，先按现场安全流程断电。

## 设备与隔离

需要至少两通道可保存原始波形/设置的示波器，足够输入分辨率和毫伏噪声能力、10×短地弹簧或适当差分探头、万用表；型号、带宽、噪声底和接地方式先记录。只有万用表可复核静态5V/3V3和节点电压，不能判定高频噪声/建立时间或ADC内部原因。
本组仅量控制侧GND2参考节点；USB与台式示波器保护地关系须先确认。禁止将示波器地夹跨接功率侧GND1与控制侧GND2，不断开示波器保护地；隔离两侧比较需要额定合适的差分/隔离测量方式，未知则不测。
保持电机UVW与六PWM物理断开、软件/硬件禁能、OST有效，不手转、不磁对齐、不接入功率驱动。24V/5V实际供电及限流、5V回灌路径由用户复核，不能根据软件60V上限提高供电。

## 一次集中测量顺序

1. 保存接线全景、仪器信息、实际母线/控制5V/3V3及参考地；复核IU=J2-17→J3-24/C9，IV=J2-18→J1-2/B2，IW=J2-20→J7-66/A4，VBUS=J2-16→J3-25/B9。只核对，不凭表自动重接。
2. 先同点双探头及短接噪声底记录，检查探头倍率/带宽/耦合/量程与接地引入差异。不要把示波器噪声当板噪声。
3. CH1测W模拟输出源端（按V1R2核对W对应输出RC节点），CH2测LaunchPad实际A4接收端；记录DC值及AC波形。输出源端的器件号需现场按图确认，不从IU的R108/C44类推W器件号。
4. 保持一通道W接收端作参考，另一通道依次测U接收端、控制5V、控制3V3，以及已审核可接触的ADC参考测点。参考点不确定则PENDING；不直接在未知VREF/去耦点长线加载探头。
5. 同步记录W与VBUS控制侧输出；分别保存短窗口和至少数十毫秒窗口，供互谱/相干性分析。可在仪器支持时从≥1MS/s、20MHz带宽限制开始，同时保留未限带对照与实际设置；这些是测量建议，不是已确认仪器参数或可排除混叠的充分条件。记录触发、采样率、存储深度和滤波，不能对不连续窗口做FFT拼接。
6. 原始波形/CSV/截图只新增归档，计算SHA；结束后记录电压和供电状态。异常供电、过压、地参考不明、输出使能迹象或探头安全不确定立即停止，不清故障继续。

## 解释范围

源端和接收端同时波动且明显高于测量噪声，支持模拟链/供电因素；接收端额外波动支持线缆/地参考/接收负载因素。与供电相关只能提示路径，需受控对照才能判断因果。
两端稳定而ADC码波动，仍需考虑示波器噪声底/带宽、非同步观测、ADC参考、通道切换和S/H时序；不能只凭“示波器看不见”就确诊ADC。当前不改ACQPS、不注入新信号，按结果决定最小受控整改。

## 后续独立功率前置验证

PWM电气与保护可以在电机/功率驱动输出保持隔离的审核夹具中分别验证，避免要求整机全资格后才验证单项。实际PWM引脚复用/输出测试需独立安全固件和明确授权，本次R03锁定OUT不能执行。
采样窗口波形必须最终同时观察审核后的门极/分流电流/ADC触发或等效可证明时刻，确定死区、消隐、建立及装载时限。无真实低能量电流/波形前不授予窗口资格。
'''

DOCS['时序合同与计时范围.md']='''# 正式控制路径实时性合同

目标120MHz、20kHz、6000ticks/50µs；45µs为优化参考，不更改原6000tick门槛。R03没有芯片耗时数据，主机mock ticks不能作为性能结果。

路径：硬件触发/S&H/转换→ADCB1 ISR prologue→flags/OVF与四路raw→ADC正常应答→QEP/Timer/TB相位→保护轮询→控制/慢速度环→RAM三相准备/提交→OVF→稀疏有界快照→PIE应答→C范围计时→wrapper尾戳→epilogue/IRET。
没有每拍前台见证/握手。前台只做安全锁定轮询和有界命令/结束发布；STOP邮件由ISR读，避免跨上下文修改控制浮点。

|执行帧|额外操作|本地审计范围|
|---|---|---|
|普通电流帧|S6_systemStep→EF/SF→PWM事务|固定路径，无完整stats|
|速度帧（每20拍）|累积速度PI、斜率/抗饱和|增量工作，未实测|
|快照帧（每256拍，最多16份）|原生128字有界快照与第二OVF读|保留实际开销，不移出预算|
|故障帧|锁存首错、保留原始帧、禁能停采|与正常调度分开，不当正常最大性能|
|结束帧|停SOCA/TBCLKSYNC/ADC中断；前台终态发布|长统计汇总在前台仅历史cmd6|
|未来对齐帧|固定定子参考和QEP保持检查|锁定OUT不可进入功率对齐；仅主机覆盖|

maxStep包含系统包装及相应SF/速度逻辑，不再标为裸SF_step耗时。maxC起于ISR内第一Timer读取至应答后C戳，仍未包含后续首错/末端收尾。maxWrapper到S6_onExit前Timer读取，仍排除S6_onExit自身和编译器epilogue/IRET。release→stage为TB/Timer估计；不是硬件触发到真实三相装载。
代码在C/wrapper≥6000、入口5400..6600或PWM提交过期时故障；未采集ADC触发前等待/总线/IRET全部边界，不因此称完整ISR/WCET通过。
未来正式验收须用一致外设事件/硬件测量覆盖S/H、ISR完成、更新装载并检查普通/速度/快照/切换/最后帧，不相加不同拍最大值，不任意扣空开销。需要合法真实profile才能验收真实影子FOC；本次PENDING不具备。
R02 wrapper4230/统计2511仅cmd6真实结果，正式电流/速度路径不执行S6_statsStep；历史SF3498也不是R03系统计算成本，禁止简单相加或推算余量。严格WCET需进一步证明输入/分支/等待上界，已测最大值只能作为测量证据。
'''

DOCS['02_CCS身份与集中导出.md']=f'''# R03 CCS身份、加载取证及集中导出合同

{HEADER}
实际本机OUT：
`D:\\aa_experimental\\codex\\F280039\\ccs_stage6_workspace\\Stage6_SENSORED_FOC\\CPU1_SENSORED_FOC_RAM\\Stage6_SENSORED_FOC.out`

当前任务只授权离线开发。本单不是自动上板授权，也不安排新的RAW/CAL。当前profile PENDING，电流/速度影子命令3/4会被拒绝，不能用WATCH写参数或证据来执行。默认推荐发令：**无**。

## 以后单独授权时的身份操作

先保全旧会话PC/SP、停止事件、调用栈、断点列表及RAM，不在异常现场直接Reset/Reload/Resume。
身份核对通过且获明确授权后，CCS Run→Load→Load Program，选择上述本次OUT（不能只Load Symbols），保存实际完整路径/控制台加载日志/SHA与关键机器码。不要沿用R02 A48D/A48A停点。
Program地址空间、16-bit Hex验读初始化{READY}、完成{DONE}各7700 FF69 0006。若有软件断点替换ESTOP0/缓存，先保存证据，不能把替换码当固件错误；不擅自修改断点恢复现场。
初始化停止需记录实际Registers PC/SP、停止事件、调用栈与启用断点列表，RAM magic53364631、build26111702、layout3、words4096、source1、power0、actuationCompiled0、backend0。初始化状态不是采集完成状态。

## 当前接口的准确语义

|gStage6Command值（u16，0x0400）|含义|当前默认结果|
|---|---|---|
|1|保留综合diag，资格不足降RAW|不安排重复执行|
|2|正式2048拍偏置|缺接口/零电流审核，拒绝|
|3|真实输入电流影子|缺物理反馈/控制profile，拒绝|
|4|真实输入速度影子|另缺磁零位/参数，拒绝|
|5|停止|活跃影子经邮箱→ISR受控停止；RAW停止|
|6|历史4096拍全统计/候选|保留兼容，不再安排CAL_02|

若未来专门授权“仅资格拒绝检查”，唯一命令为gStage6Command=3、只写一次；当前默认应run0、processed0、FOC0、status4、finalized1、rejected1，停于完成函数，不能当FOC运行通过。这不是本轮建议再做的一次实验。
当前版本没有功率运行命令。真实参数导入后须重新审核构建身份/断点，不能沿用本单SHA或直接改WATCH取得资格。

## 一次RAM导出

只有实际停机/终态证据保存后导出，不为导出恢复运行。CCS Memory Browser选**Data**、地址0xC000，**16-bit Hex**；Save Memory格式**TI Data**，Start=0xC000，Length=4096个16位word，末地址0xCFFF。总有效载荷8192八位字节，文本文件大小不等于载荷大小。
若界面为按字节计长度，必须先确认单位，不能填4096字节。保存完整导出对话框截图。已有Intel HEX格式可由配套ccs_memory读取，但必须保留格式和地址单位证据，不把DAT扩展名当格式证明。
保存原文件到新编号目录，不覆盖旧DAT。记录SHA256；`python 解析工具/parse_stage6.py 本次原始.dat --out 新建解析目录`。真实数据不得加--host-only。解析器要求精确C000..CFFF地址集合，不补缺字、不猜地址。
中途暂停/身份不符/非法状态立即保全实际PC/SP、停止事件、栈、断点、日志及当前RAM；未知就填未知，不Resume、不清故障、不重发、不自动重试。
'''

DOCS['RAM布局与评分.md']=f'''# R03 layout3

{HEADER}
继承R02 4096字存储，first C080..C0FF、last C100..C17F、16×128字snapshot C180..C97F、历史统计C980..CE7F及保留CE80..CFFF均不移动。原字段详见“历史R02参考/RAM布局与评分.md”，该文只描述旧版本；下列为新增/当前身份。

|头相对16位字偏移|R03内容|
|---|---|
|0/2/4/5|magic53364631(u32)、build26111702(u32)、layout3、4096字|
|46|systemState：2禁能、3自检、4校准、5准备、6影子、7功率准备、8对齐、9电流、10速度、11停止、12故障|
|47|七类资格位，0..127，非功率放行标志|
|55|生命周期拒绝次数u16饱和|
|58..59|RAM PWM提交次数u32|
|67|backend0=LOCKED；其他值本OUT解析拒绝|
|94..95|PWM首执行错误u32，非0必须对应终态fault|

14 power=0、66 actuationCompiled=0、source1，始终不表示实际PWM输出。fault新增8192 EXECUTOR；其他原位/容差不变。RAM loads不作真实装载评分，提交次数与staging在status2时需一致。
u32/float低16位在前、IEEE754float32，字地址，不是32位地址。

parse_stage6.py按实际build分派：R01(layout1)/R02(layout2)原版冻结解析器保留不改；R03用局部复制基础头复用旧帧校验，不修改原raw_words或原始文件，输出保持真实build/layout。
R03新增生命周期、资格位、LOCKED和执行错误一致性校验。HOST_ONLY另行标记，不能通过头source=1伪称芯片实测；真实解析不传host-only。
未调用/无样本/无该执行类别的计时使用null/N/A；0样本不作0µs或50µs余量。status2不是系统FOC/实际校准/功率/实时性通过。
旧真实R01 ON/OFF及R02完整JSON逐项兼容通过；64项QA包含损坏版本/布局、非法生命周期、后端、资格位、统计字段及候选状态反例。
'''

DOCS['主机验证与负例结果.md']='''# 本地主机回归

最终结果results/09，日志host_09。GCC标准C99、O0/O2、Wall/Wextra/Werror、fno-fast-math、ffp-contract=off、fexcess-precision=standard；两种构建与运行全部exit0，33份DAT/CSV逐字节一致。
原4096真实接口帧回放逐字段对照同一冻结SF；首次prime后4095次SF成功，另有4352拍以上连续电流影子及受控停止，未改既有公式和容差。核心数学历史独立验证资产保留，本轮不重做基本变换专项。

新增纯CPU dq简化电机模型与模拟PWM后端，超过17000拍：电流→速度→电流→参考归零停止，真实ADC/QEP接口由合成传感器驱动；模型真值只进入传感器/模型与评分，不用真值改控制角度。物理power字段始终0；超过16000次RAM模拟提交。CSV是软件分支/收敛回归，不是实物模型识别或稳定性证明。
有界256拍固定定子对齐候选测试；不自动授磁零位、不产生硬件输出。

覆盖：
- 参数缺失/NaN/非法量纲或版本、来源SHA/路径、Ld≠Lq导入拒绝。
- 生命周期非法转换、缺功率授权、过期session、资格撤销、错误操作者模式、机械/磁零位分离。
- 三相PWM完整提交、pending覆盖/重复/跳号/错session、负零以外正常有限数、NaN/Inf和占空比端点、无效采样窗口、提交边界5880ticks拒绝。
- 速度进入参数检查、Id非零速度模式拒绝、速度专用授权可停机、模式切换保持电流积分、参考斜率/限幅/反向/饱和。
- ADC缺完成/溢出/饱和、QEP错误、输入间隔/超速、bus/current范围、lock/trip/latch/SD异常、重复命令、冻结数据、计数溢出/同拍所有权冲突、首错保持。
- 原统计/方差/分段/偏置不稳/候选与资格分离、异常/溢出不半帧修改、未执行N/A。

Python解析/参数/旧真实数据兼容64项PASS。C断言数量不额外编造，实际执行记录和源断言供复核。运行包内自检只做离线Python复验，不调用CCS或执行exe。
需重建C时执行“本地主机证据/rebuild_host.py 输出新目录”，需本机GCC；输出不覆盖包内冻结参考。工具链/输入/命令见构建日志，不能将主机速度作为F280039C性能。
'''

DOCS['资源与构建审计.md']=f'''# 实际TI构建与资源

最终实际全量日志08_build.log：0 errors, 0 warnings，确实Finished building target，最终OUT与正式工程相同。早期CLI空工作区“找不到项目”日志另存开发中间轮次，未计为通过，最终使用独立构建元数据工作区按引用导入同一正式工程，无第二主工程。
TI CGT25.11.1.LTS，FPU32/EABI/O2/strict/fp_reassoc=off，原链接与外设配置不变。硬件执行后端单独对象编译exit0，最终OUT不含S6T_*或s6_pwm_target.obj；不是仅凭源码函数名判断隔离。
{HEADER}

|RAM区|已用/总字|剩余|
|---|---:|---:|
|代码RAMLSnP|11809/16384|4575|
|控制RAMGSC|546/4096|3550|
|导出RAMENC|4096/4096|0（布局固定）|
|RAMM1D|93/1016|923|
|RAMGSD|72/4088|4016|
|栈RAMM0S|分配728/728|估算运行使用344|

编译器调用图最大ISR路径276字，前台48字；加20字硬件/对齐/调用保守储备，共344/728。初始化路径194字，初始化时尚未运行控制ISR。无嵌套ADCB1假设下静态可容纳，不能称实测峰值/WCET；NMI及独立栈高水位未验证。调用图不完整函数名单（启动/时钟等）见resources.json，不假装所有路径已证明。
16份SF/EF/变换/PI/端口/板级/保护/链接/CCXML不变；10343份历史哈希保全；明确包含R02工程备份/原OUT、原ZIP/DAT和此前失败资料。完整diff包括target_review_only实现及构建排除，不隐去独立后端。
'''

DOCS['04_回传模板.md']='''# R03后续系统级测量回传模板（尚未执行）

本模板空结果表示待测，不能据本模板存在认定执行。下一次任务范围/授权内容：待用户确认。
当前冻结软件0x26111702/layout3，LOCKED；本组优先综合模拟节点/供电测量，不重复CAL_02。

|项目|记录|
|---|---|
|实验编号/日期/操作者/明确授权范围|待测|
|电机UVW、六PWM、J2/J12/S5实际状态|待核|
|USB/控制5V/母线供电、限流、前后电压|待测|
|仪器型号/校准/探头倍率/接地与隔离|待核|
|W源端实际器件测点/W接收A4测点|待核|
|U参考/5V/3V3/参考点/同步VBUS|待测|
|噪声底/耦合/带宽/采样率/深度/触发|待测|
|原始波形文件/截图/CSV/SHA|待生成|
|是否源端已有波动、接收额外噪声、供电相关|待分析；不预判因果|
|异常/停止条件/未完成项|未知|

若另行授权CCS：记录实际完整OUT路径/SHA/build/layout，Load Program日志、初始化/结束实际PC/SP/事件/栈/断点列表、唯一命令及发令前证据、Data16bit导出格式/范围4096字、原DAT及SHA。未取得的字段写未知，不用符号停点地址冒充实际PC。
分别报告真实证据、HOST_ONLY、本地静态结果、假设和下一步。不存在新RAM实验时，不填造“本次status/FOC/耗时”。
'''

DOCS['05_网页版提示词.md']=f'''请阅读本包00、结果与结论、01架构、PWM执行与采样联合合同、03示波器操作单及真实R02复核，再协助推进系统级硬件证据，不再开展RAW_03/CAL_02或Stage5 R04/R05。

本地R03已实际开发并冻结：Stage6_SENSORED_FOC / CPU1_SENSORED_FOC_RAM，build0x26111702/layout3。
OUT SHA256={SHA}。
OUT路径=D:\\aa_experimental\\codex\\F280039\\ccs_stage6_workspace\\Stage6_SENSORED_FOC\\CPU1_SENSORED_FOC_RAM\\Stage6_SENSORED_FOC.out。
初始化{READY}、完成{DONE}，两处7700 FF69 0006；命令0x0400，gStage6 C000–CFFF 4096个16位字。
可加载OUT全程LOCKED；真实PWM后端源码已TI对象编译但排除链接；没有物理PWM/FOC/ISR通过证据。
默认实际profile PENDING，命令3/4会拒绝。当前不发任何命令，不自动打开CCS、连接、加载、设置断点、改WATCH、接线、改变母线、磁对齐或试转。

真实R02 CAL_01原DAT SHA9499E85FC8095CA3034DEC825AED867E74BB71D8B959DA6C953245A2D9645EA0；4096拍完整FOC0，候选span48/44/76、拒绝11，偏置未批准。W SD13.59码，两段64点W-VBUS相关−.361/−.653只是线索，根因未定；原32码门限不变。
软件已覆盖真实反馈接口→原EF/SF→电流/速度→RAM三相事务、生命周期/限幅/停止/故障与参数导入，O0/O2 33文件一致、64项Python回归、TI0错误0警告；实际增益/极性/磁零位/采样窗/保护/真实时序均待审。

请先核对我实际拥有的示波器/探头和参考地条件，说明一次联合测W源端/接收端/U参考/5V/3V3/VBUS的测量范围与限制。任何物理接线/上电需我另行明确授权，再逐阶段指导；不假设仪器参数，不跨接GND1/GND2，不重跑已完成统计来代替根因测量。
本提示词不是硬件授权。不要把本地主机/构建通过写成真实电机、实际PWM或WCET通过。保留历史SFfault8、SMO和Stage5三轮失败。
'''

def main():
    for name,body in DOCS.items():(H/name).write_text(body,encoding='utf-8')
    (H/'README.md').write_text(DOCS['00_使用说明.md'],encoding='utf-8')
    root=R/'README.md';s=root.read_text(encoding='utf-8-sig')
    entry='''2026-11-17 Stage6 R03系统集成已完成本地软件与构建：build26111702/layout3，原EF/SF与真实反馈、速度环、生命周期及RAM PWM事务集成；真实global-load后端独立对象编译并排除OUT。见[结果](软件验证/2026-11-17_Stage6_R03系统集成/结果与结论.md)、[综合波形测量草案](软件验证/2026-11-17_Stage6_R03系统集成/03_综合示波器测量操作单.md)、[完整包](网页版交接/Stage6_SENSORED_FOC_R03_2026-11-17.zip)。O0/O2 33文件一致、64项解析/参数/旧真实兼容通过，TI0错误0警告；OUT全过程锁定，物理参数PENDING。真实R02 CAL_01完整但候选span48/44/76拒绝11，不导入。关闭重复RAW/CAL与Stage5专项；下一步需另行授权综合波形测量，不自动CCS/接线/上电/试转。以下为历史时点记录。\n\n'''
    if entry not in s:s=s.replace('## 当前从哪里开始\n\n','## 当前从哪里开始\n\n'+entry)
    root.write_text(s,encoding='utf-8')
    print('Wrote',len(DOCS),'R03 review documents from actual image identity')

if __name__=='__main__':main()
