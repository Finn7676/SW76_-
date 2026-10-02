# 长度 4 先吃珠、再分裂修复

用户截图：Screenshot 2026-10-02 at 13.16.12.png；用户确认 ID 2，第 5 回合。未提供这张截图的新对局链接，因此此处验证的是截图局面重建，没有核对该局实际 MOVE/SPLIT 日志。

## 原因

shortPearlEscapeAction 和最后动作保护 preservesShortPearlEscape 都要求“没有任何可见敌人”。因此长度 4、面前有珍珠、尾部可逃时，隔墙敌人也会让吃珠脱困失效，接着触发开局 SPLIT 2。重建中无敌人选 MOVE N；加入隔墙敌人，旧代码改为 SPLIT 2。

## 修复

完全可见、无人占据、无分叉、单入口的短死胡同仍可先吃珠。普通单步移动保留原来的头作为颈部，封住通道唯一入口，外部敌人只能撞身体，不能穿过它撞新头；不再因为隔墙可见敌人取消这个已证明的路线。该规则只适用于原有最多 3 格的短通道，不放宽一般开放地形的敌人风险判断。

最终动作保护同步保留该 MOVE，不再将它改回 SPLIT 2。吃完长度 4→5，再分出尾部 3、头保留 2。短通道脱困取消人为 targetPopulation 条件，延续此前忽略人为人口目标的要求，保留硬 unit_limit。仍检查尾部逃路、身体/地形阻挡、Queen 禁止新入死胡同、队友出口和珍珠预约。

## 验证

21 组 C++ 回归、7 个历史输入 ASan/UBSan 通过；专项截图重建也通过 ASan/UBSan。覆盖 ID 2 / round 5，0/1/2 个隔墙敌人、人口 6/50、吃后 SPLIT 3，以及通道被占、尾部封闭、第二入口有敌人的拒绝情况。

官方 1.2.3 引擎的两个队伍 WASM 快照验证完整 main.cpp 输出 MOVE N，再 SPLIT 3；实际长度 4→5→2，尾部子龙得到 3。ROUND 标签注入 5/6，引擎物理和顺序不变；隔墙敌人由脚本控制，不是完整比赛胜率测试。结果见 verification/pearl_before_split_results.json。

前面的 DEADEND_SPRINT_FIX_20261002.md 与相关结果文件记录前一版验证；本次源码摘要与结果见 verification/pearl_before_split_validation.json。
