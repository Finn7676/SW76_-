# 结果与主张边界

| 主张 | 判定 | 直接证据 |
|---|---|---|
| relay 在本次同条件验证中胜场更多 | 支持：25/60 对 17/60，净增8胜 | final_summary.json；两组 results.csv |
| relay 已经对 V3 胜多负少 | 不支持：25胜35负 | relay_final/results.csv |
| 原版主要只是早期死亡更多 | Schooltime 切片不支持：V3 死亡损失更高，同时增长更多 | DIAGNOSIS_BASELINE.md；economy_diagnostics_raw.json |
| 无证据门户认领占用全部广播方向 | 源码支持；胜率不能细分归因到单个子改动 | 原版 main.cpp；relay main.cpp |
| 更多移动覆盖必然有更好经济 | 不支持：第200轮独特头格较多而总长落后 | 修正后的六局 Schooltime 计数 |
| 提前冠军或当前 antiram 可提高总体胜率 | 开发测试不支持 | economy_champion 8/20；antiram 9/20；原版9/20 |
| relay 改善所有地图 | 不支持：Autarky、Slithery Fight 各少1胜 | final_summary.json 的 by_map |
| 具有统计显著性或跨对手优势 | 未检验，不作此主张 | 仅一个对手、三个最终种子、描述统计 |
| code 4 必然表示 V3 程序异常 | 不支持：V3 明确实现并测试主动 NONE；逐事件分支未追踪 | V3 planner.hpp:29–35、528；action.hpp:44–45；v3_tests.cpp.txt |
| 最终来源完整且无已识别进程故障 | 支持；与零比赛内无动作死亡不同 | final_verification.json 的 artifact_integrity_passed、runtime_failure_free 均为 true |

开发筛选（8候选各20局）、历史诊断、提前读取的辅助原版组和最终两组各60局分开记录。最终完整性审计与代码复核见 REPORT.md；不能仅凭汇总器分母推断实验完成。

验证器的严格 passed=false 原样保留，因为它额外要求两队均零无动作死亡。18 次此类事件均在 V3 一侧，不能自动视为程序故障，也没有从正式结果中删除。
