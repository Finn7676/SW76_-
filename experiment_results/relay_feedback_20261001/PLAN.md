# 用户回放反馈：三个策略模块

日期2026-10-01。主要底座为tyhh_qos_relay_counter；同时保留当前
tyhh_qos_protect_relay作为对照。用户要求每版全场景、交换先后手共40次：
V3_portal与xxqztest各十张官方地图×A/B，每对手20局。固定新种子76001，
不与先前开发或最终样本合并。所有完整对局保留；无动作死亡不单独视为
运行失败。没有论文级多种子主张：本轮结果只是固定40局的对照筛选。

## 矩阵

| ID | 父版 | 变化模块 | 预期与风险 |
|---|---|---|---|
| B_relay | 当前relay | 无 | 新种子同条件参照 |
| B_counter | 当前counter | 无 | 三个单项直接参照 |
| H1 safeportal | counter | 长龙仅过当前观测证明安全的门户 | 少未知出口/占用落点损失，可能丢失采集机会 |
| H2 feednear | counter | 更早且允许近等长的有接收条件献祭 | 提高最长及回收率，可能损失总长与行动数 |
| H3 reclaim | counter | 换头看第三方收尸机会并优先收集实际死亡珠 | 减少为敌人留珠交易，可能错过阻断敌人的攻击 |
| H4 combination | 已测有益单项 | 每次再加一个已测试模块 | 检验兼容，不预设可加性 |
| H5 parent check | relay | 在relay上复制有益的单项/所选组合 | 检验底座差异 |

每个模块在对应EXPERIMENT.md登记具体策略、观测限制和验证。父版及对手
生产源码均不修改。模块配套逻辑作为一个因子，不能把各子步骤单独宣称
为因果效应。单项测试结果不支持时只保留负结果，不展开无归因阈值搜索。

## 评价规则

主要看分别对两个对手的胜/负/平，与父版匹配条件比较。
“有益”筛选要求两对手胜场均不低于父版，且至少一方提高；同时记录
负→胜、胜→负及逐地图/先后手退化。选择优先较弱对手胜数，然后总胜数，
再少退化地图、少改动模块。单seed不作显著性或稳定统治结论。
若无满足筛选版本，如实报告无有益方案，不依成绩修改口径。

次要证据：终局最长、总长、人口；门户保护实际拒绝/改向日志；
献祭/回收触发；换头被拒和真正可见死亡珠收集触发。比赛胜负优先于
触发次数。最多选少量明确回放片段说明机制，不推断每个聚合死亡的意图。

## 执行与资源

使用原run_suite.py，每版冻结源码、编译一次、40局并记录sources/run.json/
results.csv/summary.json及全量回放和日志。复用evaluate.py验证双方、
种子、地图、唯一完整条件、源码快照、二进制、日志及回放。jobs合计最多4。
初轮两基线+三单项=200局，最多两次组合及一父版检查总计不超过320局；
据此前耗时估计约15–35分钟加实现，回放约1–3GB，初始余量33GB。
本地CPU，无GPU、付费API或外部提交。

命令模板，从仓库根执行，输出目录必须不存在：

~~~sh
PATH=/tmp/clang-toolchain:$PATH python3 experiment_results/protect_local_20260927/run_suite.py --bot tyhh_test/tyhh_qos_relay_counter --opponents tyhh_test/V3_portal tyhh_test/xxqztest --maps all --seeds 76001 --jobs 2 --output experiment_results/relay_feedback_20261001/counter_baseline
~~~
