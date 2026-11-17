# 板级 HAL 对照表 v0.2：锁定配置

日期：2026-11-05。承接[连接方案 v0.1](LAUNCHXL-F280039C_野火EBF410194V1R2_引脚连接方案_v0.1.md)与[HAL 对照表 v0.1](板级HAL对照表_v0.1.md)。本表记录 `../ccs_stage1_workspace/f280039c_wildfire_stage1_locked/` 的**实际软件配置**。用户明确决定跳过 ESD 测试和断电通断等静态检查；这两项记为“跳过”，不记为“通过”。没有连接驱动板、烧录或试转证据。

| 功能 | J2 / J12 候选接口 | Stage 1 实现 | 仍待验证或待实现 |
|---|---|---|---|
| 六路 ePWM | 野火 J2-3～8；LaunchPad GPIO0/1、2/3、10/11 | 内部 ePWM1/2/6 均配置上/下计数、比较 A、互补死区候选值；所有模块计数器 `STOP_FREEZE`，TBCLKSYNC 关闭，并强制 OST。六个封装脚仍是 **GPIO 输出低**，未复用到 ePWM。 | 20 kHz / 60 TBCLK 只是软件候选；光耦延时、极性、死区及同步波形均未测。启能路径尚未写入。 |
| 三相电流 ADC | J2-17/18/20 → ADCC C9、ADCB B2、ADCA A4 | ADC A/B/C 配置预分频和 SOC0，触发源均为 ePWM1 SOCA；未启用 SOCA，因此无周期采样。 | analog mux、三相采样窗、偏置/比例/符号、ADC VREF 与过量程。F28003x DriverLib 没有 `ADC_setMode()`，本机编译后按该器件固定 ADC 模式处理。 |
| 母线 ADC | J2-16 → ADCB B9 | ADCB SOC1 同样绑定 ePWM1 SOCA，排在 B2 的 SOC0 之后；触发源禁用。 | `POWER/37+1.65 V` 是图纸式，未标定量程；采样延迟与过压阈值未定。 |
| SD_A | 野火 J2-13 → GPIO8 | GPIO8 配为输入，不提供使能电平。 | 许可/禁能极性、上电和复位行为未测。 |
| CMPSS → Trip Zone | 电流物理脚候选 CMPSS1/2/3 → ePWM X-BAR TRIP4 → ePWM1/2/6 | 三个 CMPSS 采用 DAC 占位阈值（高 0、低 4095）及异步输出，TRIP4 对三路 OR，三个 ePWM 配置 DCAEVT1 跳闸；另有每路软件 OST 锁存。占位阈值仅为保持锁定，**不是过流整定值**。 | Stage 1/2 相别映射有误：实际应是 IU→CMPSS1、IV→CMPSS3、IW→CMPSS2，且当时未明确配置模拟 mux。Stage 3 软件已修正；CMPSS/Trip 极性、传播延时、六路故障电平、复位策略仍未实测。不能把此构建作为保护验收。 |
| J12/eQEP1 | 2026-11-16针序复核：1=A/GPIO40、2=B/GPIO41、3=I/Z/GPIO59、4=+5V_MCU、5=GND；1脚方形标识 | GPIO 复用为 eQEP1 A/B/Index，解码与计数器配置完成；没有角度标定或参与闭环。 | S5 左侧 0/down、5 V 供电、电平、分辨率、方向和零位仍未核对。 |
| 控制侧供电 | 野火 J2-1、GND2 | 代码不控制供电；本次未连接。 | 5V_IN 预算、反灌、GND1/GND2 和 JP1/JP2/JP8 实物条件没有静态测量；按用户决定跳过相应检查，不能推定正确。 |

该工程与 TI Universal Motor Control Lab 的区别不变：没有 FOC、电机参数、观测器、PLL、电流环、记录链，也没有 TI 主工程源码。下一次若准备物理接线，至少需要明确现有未测条件对电源、信号极性和故障行为的影响；本表不构成接线许可。
