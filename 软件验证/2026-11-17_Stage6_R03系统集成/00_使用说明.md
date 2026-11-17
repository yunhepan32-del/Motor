# Stage6 R03 系统集成冻结交付

工程 Stage6_SENSORED_FOC；配置 CPU1_SENSORED_FOC_RAM；build **0x26111702**；layout **3**。
OUT SHA256：`5F0DCA904882D648AE94680A8E7B078DD6B936E9F6E26FDFC0CF627219743827`。
初始化 `0xAE18`，完成 `0xAE15`；均为 `7700 FF69 0006`（NOP/SPM #0/LRETR）。
命令 gStage6Command=`0x0400`，结果 gStage6=`0xC000–0xCFFF`，4096个16位字。

本地状态：**LOCAL_SOFTWARE_AND_BUILD_VERIFIED**。本包没有R03芯片实测，不是实际电机运行放行包。

已完成实际源码：真实ADC/eQEP输入与原EF/SF电流环、速度外环集成；三相PWM事务、运行生命周期、独立资格及有界操作者授权、受控停止、首错锁存；电机参数工具；真实目标PWM后端独立对象编译。
可加载OUT仅使用LOCKED后端，全程禁能。真实CMPA/global-load后端位于target_review_only，构建排除且最终MAP/符号无该对象/入口。无清OST、GPIO功率复用或解锁提供者。

先读《结果与结论》《01_控制架构与源码审计》《03_综合示波器测量操作单》；《02_CCS身份与集中导出》仅保存实际身份和后续验收合同，本轮不自动安排加载或新CAL实验。
旧RAW/CAL与Stage5专项关闭迭代，原能力/证据保留；不再开展相同CAL_02，不重测IU，不并行SMO。

包内包含正式工程源码/配置、冻结OUT/MAP/反汇编/链接信息、独立后端对象、完整diff、最终O0/O2日志及数据、原R01/R02真实原件、三代解析器、实际PENDING及HOST_ONLY参数、校准与参数工具、审计和SHA256SUMS。
先执行 `python 校验包.py`，再执行 `python 运行包内自检.py`；两者仅离线读取，测试输出写临时目录。主机C回归重建入口见《主机验证与负例结果》。
只有另获用户授权且满足硬件前置条件，才可实施示波器测量、物理校准、PWM/保护独立验证或电机动作。本轮未进行任何连接、加载、写目标RAM、上电或试转。
