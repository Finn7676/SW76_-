# 双对手独立验证：不建议替换原版

本版本基于tyhh_qos_protect_relay，仅增加长度2主动撞头的收益和末单位约束。
生产源码在评测前冻结，没有根据最终结果改动。

最终条件：官方十图，交换A/B，未用于调参的三个种子74001、74002、74003。

| 对手 | 原relay | 本版本counter |
|---|---:|---:|
| V3_portal |26胜34负（43.3%）|25胜35负（41.7%）|
| xxqztest |26胜34负（43.3%）|22胜38负（36.7%）|

未实现同时战胜两个对手；开发集上的改善没有在独立最终样本重现。
保留此目录用于追溯实验，不将其作为优于原版的生产建议。
原relay、V3_portal、xxqztest均保持不变。旧EXPERIMENT.md中的继承结果
属于父版本，不能作为本counter的成绩。

完整实验报告、源码冻结、失败版本、回放与验证数据：
../../experiment_results/relay_dual_20260929/REPORT.md
../../experiment_results/relay_dual_20260929/REPLAYS.md
../../experiment_results/relay_dual_20260929/final_verification.json
