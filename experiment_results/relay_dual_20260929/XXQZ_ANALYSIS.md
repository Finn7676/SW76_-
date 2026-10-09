# xxqztest 与 relay 的可独立检验策略差异

日期：2026-09-29。只读取开发种子 73001；未读取 74001 及以后最终验证数据。

## 源码证据与边界

- xxqztest/planner.hpp:607–623 的 adjacentRam 要求当前可见敌身段数至少 3、己方人口大于 1；它对短的 ID-selected champion 仍允许以小换大。relay/main.cpp 原 981–985 行对 LEN==2 且存在相邻敌首直接主动对撞，绕过长度收益和最后单位保护。
- xxqztest/planner.hpp:191–205 将有利头撞和冲刺置于一般危急评分之前；:222–225 在350轮前允许切分至母体长度2。:883–947 的 sprintRam 包含可见路径和逐步珍珠支付模拟。这些构成多个独立差异，本候选不同时移植。
- xxqztest/planner.hpp:831–855 还有尾部短攻击者战术切分，observer.hpp:181–212 有可见死亡珍珠线索。它们需要额外时序/观测验证，未纳入H3。
- xxqztest 的献祭文档与当前常量并非完全一致：core.hpp 的 V4_SUMMON_ROUND=350、V3_SACRIFICE_START_ROUND=400。本文以当前代码为准，献祭与门户模块由其他实验负责。

## 开发基线

baseline_dev/results.csv：当前 relay 对 V3 为7胜13负，对xxqztest为6胜14负；每对手20局，十图双侧，seed73001。尚不能以单种子成绩建立稳定强弱结论。

针对 Queen Of Spades、Trophy 对xxqztest的四个失败条件，复用既有守恒重建器，得到 relay 共147次头碰撞死亡、176次总死亡。原始逐局数据、回放SHA和守恒/最终阵容检查保存于 tyhh_test/tyhh_qos_relay_counter/verification/baseline_combat_slices.json。各局 head 死亡依次为 Queen B=10、Queen A=41、Trophy A=40、Trophy B=56。数据说明应检查头碰撞决策，但没有区分主动与被动碰撞，也没有证明无收益交易是落败主因；不据此宣称因果。

## 单项候选 H3

目录 tyhh_test/tyhh_qos_relay_counter，只替换长度2主动对撞的收益门槛：可见敌长至少3，且己方仍有第二条龙。目标是在短龙密集对抗中避免无明确收益的主动等长交易；有可见收益的攻击以及无安全出口时的原有最后手段不变。

该假设比同时引入冲刺搜索、短敌回避、战术切分和死亡珍珠路由更容易被单独检验。参数3不是事后调参：它是长度2发生严格可见以小换大的最小整数门槛，并与xxqz现行选择一致。代价可能是减少相互淘汰、让敌方资源收集者存活；改善并无保证。不能把可见段数不足3理解为敌人实际只有2段。

候选的 EXPERIMENT.md 在比赛前登记边界、正反预期和两对手测试。原始relay、xxqztest与V3保持不变；不运行全量比赛，不改原评测口径，不使用最终保留种子。
