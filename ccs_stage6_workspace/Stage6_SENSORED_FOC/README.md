# Stage6_SENSORED_FOC R03

0x26111702 / layout3 / CPU1_SENSORED_FOC_RAM。
真实反馈→EF/SF→电流环/速度环→RAM PWM事务、生命周期和故障管理已集成。
可加载OUT全过程锁定；target_review_only为独立编译的真实执行后端，明确排除链接。
实际参数/电流增益/磁零位/保护/采样窗口/实时性均需独立审核；当前默认profile PENDING，cmd3/4拒绝。
完整交付：网页版交接/Stage6_SENSORED_FOC_R03_2026-11-17；本地结果：软件验证/2026-11-17_Stage6_R03系统集成/结果与结论.md。
不重开RAW/CAL及Stage5专项。不自动连接、加载、接线、上电、磁对齐或试转。
