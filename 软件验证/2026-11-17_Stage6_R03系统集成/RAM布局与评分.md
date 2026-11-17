# R03 layout3

工程 Stage6_SENSORED_FOC；配置 CPU1_SENSORED_FOC_RAM；build **0x26111702**；layout **3**。
OUT SHA256：`5F0DCA904882D648AE94680A8E7B078DD6B936E9F6E26FDFC0CF627219743827`。
初始化 `0xAE18`，完成 `0xAE15`；均为 `7700 FF69 0006`（NOP/SPM #0/LRETR）。
命令 gStage6Command=`0x0400`，结果 gStage6=`0xC000–0xCFFF`，4096个16位字。

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
