# PWM执行、采样与应用时刻

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
