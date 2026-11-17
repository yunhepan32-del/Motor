# R03 CCS身份、加载取证及集中导出合同

工程 Stage6_SENSORED_FOC；配置 CPU1_SENSORED_FOC_RAM；build **0x26111702**；layout **3**。
OUT SHA256：`5F0DCA904882D648AE94680A8E7B078DD6B936E9F6E26FDFC0CF627219743827`。
初始化 `0xAE18`，完成 `0xAE15`；均为 `7700 FF69 0006`（NOP/SPM #0/LRETR）。
命令 gStage6Command=`0x0400`，结果 gStage6=`0xC000–0xCFFF`，4096个16位字。

实际本机OUT：
`D:\aa_experimental\codex\F280039\ccs_stage6_workspace\Stage6_SENSORED_FOC\CPU1_SENSORED_FOC_RAM\Stage6_SENSORED_FOC.out`

当前任务只授权离线开发。本单不是自动上板授权，也不安排新的RAW/CAL。当前profile PENDING，电流/速度影子命令3/4会被拒绝，不能用WATCH写参数或证据来执行。默认推荐发令：**无**。

## 以后单独授权时的身份操作

先保全旧会话PC/SP、停止事件、调用栈、断点列表及RAM，不在异常现场直接Reset/Reload/Resume。
身份核对通过且获明确授权后，CCS Run→Load→Load Program，选择上述本次OUT（不能只Load Symbols），保存实际完整路径/控制台加载日志/SHA与关键机器码。不要沿用R02 A48D/A48A停点。
Program地址空间、16-bit Hex验读初始化0xAE18、完成0xAE15各7700 FF69 0006。若有软件断点替换ESTOP0/缓存，先保存证据，不能把替换码当固件错误；不擅自修改断点恢复现场。
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
