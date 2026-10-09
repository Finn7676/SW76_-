# 局部队友保护实验结果（2026-09-27）

## 完成范围

直接修改 tyhh_test/tyhh_qos_protect_route。生产 tyhh 的 main.cpp、helper.hpp、bot.toml 与 V3_portal/main.cpp 输入哈希均未变化；比赛源码快照与当前 main.cpp 哈希相同。
完整覆盖10张官方地图；3个固定种子（20260927、20260928、20260929），每张地图交换A/B双方，每个对手60局，总计120局。120局全部完成，运行故障0、非法动作死亡0。
19组白盒场景测试通过 ASan/UBSan；测试日志见 protection_tests.log。LSan因沙箱限制关闭。

## 本轮实现

1. 删除旧 M_INTENT 硬预约。动作执行后才发送声呐，旧消息中的本次落点不能称为未来路线。
2. 原珍珠、敌情、传送门和前沿广播仍走原动作后朝向。其他三个空闲方向用独立uint64状态消息；四向房间占用声明优先，不被覆盖。
3. 状态包含己方标志、发送者ID、发送回合、动作后头部及长度。严格按ID行动顺序检查有效期，再用当前可见己方头部核验；未知门户不虚构落点。
4. 较长队友需至少长度6，且比本龙至少长2节。动作将堵死其全部已知出口时才按强冲突处理；有其他出口的直行重叠最多扣48分，连续让路时降至24分，连续两次后停止软扣分。
5. 普通移动、传送、头碰头、冲刺和分裂全部先提出动作，再统一审查。冲刺核对中途形成的身体，分裂核对尾格继续占用。
6. 局部路线冲突只有找到自身可行的安全替代才改道；无安全替代保留原动作。明确撞己方头时优先合法分裂或其他出口；生路耗尽时避免同时杀死友军头。
7. 本轮未调整原有探索、珍珠、传送门评分及自适应切换阈值；新保护逻辑不使用地图名称。

## 总结果

| 对手 | 胜 / 负 / 平 | 胜率 | 3个种子的胜场（各20局） | 种子胜率均值 ± 样本标准差 |
|---|---:|---:|---|---:|
| tyhh | 32 / 28 / 0 | 53.3% | 8 / 13 / 11 | 53.3% ± 12.6个百分点 |
| V3_portal | 25 / 35 / 0 | 41.7% | 9 / 8 / 8 | 41.7% ± 2.9个百分点 |

对 tyhh 的小幅领先在不同种子间波动较大；对 V3_portal 仍然处于劣势。本轮结果不能证明整体稳定增强，也不能与旧版不同种子、不同场次的历史胜率直接做因果比较。

## 逐图结果

每格均为实验版的 胜 / 负 / 平，每个对手每图6局。

| 地图 | 对 tyhh | 对 V3_portal |
|---|---:|---:|
| Autarky | 3 / 3 / 0 | 3 / 3 / 0 |
| Default | 3 / 3 / 0 | 1 / 5 / 0 |
| Devil | 3 / 3 / 0 | 5 / 1 / 0 |
| Portals | 2 / 4 / 0 | 3 / 3 / 0 |
| Prisoners Dilemma | 3 / 3 / 0 | 2 / 4 / 0 |
| Queen Of Spades | 3 / 3 / 0 | 1 / 5 / 0 |
| Schooltime | 3 / 3 / 0 | 0 / 6 / 0 |
| Slithery Fight | 4 / 2 / 0 | 2 / 4 / 0 |
| Trauma | 5 / 1 / 0 | 3 / 3 / 0 |
| Trophy | 3 / 3 / 0 | 5 / 1 / 0 |

## 回放诊断

同种子 Devil（20260927，实验方A）修前/修后对照共7834条动作、位置、分裂、死亡记录逐条相同，均142轮被淘汰。新保护 PROTECT、PROTECT_KEEP、PROTECT_DIRECT 均未触发；前50轮的96个独特头格、前100轮的113个独特头格也相同。因此，不能把这局的绕圈直接归因于保护机制，该局仍需要检查原寻路及扩张行为。
[修前 Devil 回放](smoke/before_devil_A.replay)；[修后 Devil 回放](smoke/devil_A.replay)；[逐事件对照](devil_paired_compare.json)。

120局前50轮累计没有局部路线让路日志 PROTECT 或 PROTECT_KEEP；有62次直接友军碰撞检查后的紧急替代日志。该统计说明本次实现的早期介入范围较窄，不等同于证明探索质量已经提高。

| 对手 | 路线改道 PROTECT | 无安全替代 KEEP | 直接碰撞紧急处理 DIRECT |
|---|---:|---:|---:|
| tyhh | 149 | 87 | 978 |
| V3_portal | 124 | 82 | 1185 |

以上是触发日志次数，不能当作成功救下队友的数量。独特头格和重复率仅衡量团队头部移动，不代表实际探索视野覆盖。完整定义与逐局数据见 metrics_full.json / metrics_full.csv。

## 当前主要失败切片

对 V3_portal 的35负中，25局在500轮按长度判负，10局被淘汰。Schooltime 0胜6负，6局均在500轮最长龙比较落后；Default 1胜5负；Queen Of Spades 1胜5负（其中4场被淘汰）。保护修复尚未解决这些竞争力问题。

| Schooltime种子 | 实验方 | 实验版最长 | V3最长 | 实验版总长度 | V3总长度 |
|---|---|---:|---:|---:|---:|
| 20260927 | A | 20 | 23 | 195 | 267 |
| 20260927 | B | 12 | 20 | 147 | 285 |
| 20260928 | A | 14 | 23 | 157 | 250 |
| 20260928 | B | 13 | 19 | 170 | 222 |
| 20260929 | A | 23 | 34 | 192 | 272 |
| 20260929 | B | 18 | 32 | 185 | 245 |

## 复现与证据

- 协议与冻结假设：[PLAN.md](PLAN.md)
- 总比赛表：[results.csv](full_v1/results.csv)
- 完整回放与各局stdout：[full_v1](full_v1/)
- 输入/地图哈希与编译源快照：[run.json](full_v1/run.json)、[sources](full_v1/sources/)
- 回放重建与保护计数：[metrics_full.csv](metrics_full.csv)
- 场景测试：[protection_tests.cpp](protection_tests.cpp)、[测试输出](protection_tests.log)
- 基线未改与测试源码一致性：[verification.json](verification.json)

复现命令（输出目录须为新目录）：

    PATH=/tmp/clang-toolchain:$PATH python3 experiment_results/protect_local_20260927/run_suite.py --seeds 20260927 20260928 20260929 --maps all --jobs 2 --output experiment_results/protect_local_20260927/reproduction

## 全部失败回放

### 对 tyhh

| 地图 | 种子 | 实验方 | 结束轮数 | 回放 |
|---|---:|---|---:|---|
| Autarky | 20260927 | A | 500 | [0001_tyhh_autarky_seed20260927_A.replay](full_v1/0001_tyhh_autarky_seed20260927_A.replay) |
| Autarky | 20260927 | B | 500 | [0002_tyhh_autarky_seed20260927_B.replay](full_v1/0002_tyhh_autarky_seed20260927_B.replay) |
| Default | 20260927 | A | 500 | [0003_tyhh_default_seed20260927_A.replay](full_v1/0003_tyhh_default_seed20260927_A.replay) |
| Devil | 20260927 | A | 142 | [0005_tyhh_devil_seed20260927_A.replay](full_v1/0005_tyhh_devil_seed20260927_A.replay) |
| Portals | 20260927 | A | 500 | [0007_tyhh_portals_seed20260927_A.replay](full_v1/0007_tyhh_portals_seed20260927_A.replay) |
| Prisoners Dilemma | 20260927 | B | 206 | [0010_tyhh_dilemma_seed20260927_B.replay](full_v1/0010_tyhh_dilemma_seed20260927_B.replay) |
| Queen Of Spades | 20260927 | A | 73 | [0011_tyhh_queen_of_spades_seed20260927_A.replay](full_v1/0011_tyhh_queen_of_spades_seed20260927_A.replay) |
| Schooltime | 20260927 | A | 500 | [0013_tyhh_schooltime_seed20260927_A.replay](full_v1/0013_tyhh_schooltime_seed20260927_A.replay) |
| Slithery Fight | 20260927 | A | 500 | [0015_tyhh_slithery_fight_seed20260927_A.replay](full_v1/0015_tyhh_slithery_fight_seed20260927_A.replay) |
| Slithery Fight | 20260927 | B | 500 | [0016_tyhh_slithery_fight_seed20260927_B.replay](full_v1/0016_tyhh_slithery_fight_seed20260927_B.replay) |
| Trauma | 20260927 | B | 500 | [0018_tyhh_trauma_seed20260927_B.replay](full_v1/0018_tyhh_trauma_seed20260927_B.replay) |
| Trophy | 20260927 | A | 407 | [0019_tyhh_trophy_seed20260927_A.replay](full_v1/0019_tyhh_trophy_seed20260927_A.replay) |
| Default | 20260928 | B | 362 | [0024_tyhh_default_seed20260928_B.replay](full_v1/0024_tyhh_default_seed20260928_B.replay) |
| Devil | 20260928 | B | 159 | [0026_tyhh_devil_seed20260928_B.replay](full_v1/0026_tyhh_devil_seed20260928_B.replay) |
| Portals | 20260928 | B | 500 | [0028_tyhh_portals_seed20260928_B.replay](full_v1/0028_tyhh_portals_seed20260928_B.replay) |
| Prisoners Dilemma | 20260928 | B | 393 | [0030_tyhh_dilemma_seed20260928_B.replay](full_v1/0030_tyhh_dilemma_seed20260928_B.replay) |
| Queen Of Spades | 20260928 | A | 157 | [0031_tyhh_queen_of_spades_seed20260928_A.replay](full_v1/0031_tyhh_queen_of_spades_seed20260928_A.replay) |
| Schooltime | 20260928 | A | 500 | [0033_tyhh_schooltime_seed20260928_A.replay](full_v1/0033_tyhh_schooltime_seed20260928_A.replay) |
| Trophy | 20260928 | A | 302 | [0039_tyhh_trophy_seed20260928_A.replay](full_v1/0039_tyhh_trophy_seed20260928_A.replay) |
| Autarky | 20260929 | B | 500 | [0042_tyhh_autarky_seed20260929_B.replay](full_v1/0042_tyhh_autarky_seed20260929_B.replay) |
| Default | 20260929 | B | 500 | [0044_tyhh_default_seed20260929_B.replay](full_v1/0044_tyhh_default_seed20260929_B.replay) |
| Devil | 20260929 | A | 127 | [0045_tyhh_devil_seed20260929_A.replay](full_v1/0045_tyhh_devil_seed20260929_A.replay) |
| Portals | 20260929 | A | 500 | [0047_tyhh_portals_seed20260929_A.replay](full_v1/0047_tyhh_portals_seed20260929_A.replay) |
| Portals | 20260929 | B | 500 | [0048_tyhh_portals_seed20260929_B.replay](full_v1/0048_tyhh_portals_seed20260929_B.replay) |
| Prisoners Dilemma | 20260929 | B | 353 | [0050_tyhh_dilemma_seed20260929_B.replay](full_v1/0050_tyhh_dilemma_seed20260929_B.replay) |
| Queen Of Spades | 20260929 | A | 213 | [0051_tyhh_queen_of_spades_seed20260929_A.replay](full_v1/0051_tyhh_queen_of_spades_seed20260929_A.replay) |
| Schooltime | 20260929 | A | 500 | [0053_tyhh_schooltime_seed20260929_A.replay](full_v1/0053_tyhh_schooltime_seed20260929_A.replay) |
| Trophy | 20260929 | B | 184 | [0060_tyhh_trophy_seed20260929_B.replay](full_v1/0060_tyhh_trophy_seed20260929_B.replay) |

### 对 V3_portal

| 地图 | 种子 | 实验方 | 结束轮数 | 回放 |
|---|---:|---|---:|---|
| Default | 20260927 | A | 500 | [0063_V3_portal_default_seed20260927_A.replay](full_v1/0063_V3_portal_default_seed20260927_A.replay) |
| Default | 20260927 | B | 500 | [0064_V3_portal_default_seed20260927_B.replay](full_v1/0064_V3_portal_default_seed20260927_B.replay) |
| Portals | 20260927 | A | 500 | [0067_V3_portal_portals_seed20260927_A.replay](full_v1/0067_V3_portal_portals_seed20260927_A.replay) |
| Portals | 20260927 | B | 500 | [0068_V3_portal_portals_seed20260927_B.replay](full_v1/0068_V3_portal_portals_seed20260927_B.replay) |
| Prisoners Dilemma | 20260927 | B | 235 | [0070_V3_portal_dilemma_seed20260927_B.replay](full_v1/0070_V3_portal_dilemma_seed20260927_B.replay) |
| Queen Of Spades | 20260927 | A | 500 | [0071_V3_portal_queen_of_spades_seed20260927_A.replay](full_v1/0071_V3_portal_queen_of_spades_seed20260927_A.replay) |
| Queen Of Spades | 20260927 | B | 239 | [0072_V3_portal_queen_of_spades_seed20260927_B.replay](full_v1/0072_V3_portal_queen_of_spades_seed20260927_B.replay) |
| Schooltime | 20260927 | A | 500 | [0073_V3_portal_schooltime_seed20260927_A.replay](full_v1/0073_V3_portal_schooltime_seed20260927_A.replay) |
| Schooltime | 20260927 | B | 500 | [0074_V3_portal_schooltime_seed20260927_B.replay](full_v1/0074_V3_portal_schooltime_seed20260927_B.replay) |
| Slithery Fight | 20260927 | A | 500 | [0075_V3_portal_slithery_fight_seed20260927_A.replay](full_v1/0075_V3_portal_slithery_fight_seed20260927_A.replay) |
| Trauma | 20260927 | A | 500 | [0077_V3_portal_trauma_seed20260927_A.replay](full_v1/0077_V3_portal_trauma_seed20260927_A.replay) |
| Autarky | 20260928 | A | 500 | [0081_V3_portal_autarky_seed20260928_A.replay](full_v1/0081_V3_portal_autarky_seed20260928_A.replay) |
| Autarky | 20260928 | B | 500 | [0082_V3_portal_autarky_seed20260928_B.replay](full_v1/0082_V3_portal_autarky_seed20260928_B.replay) |
| Default | 20260928 | A | 500 | [0083_V3_portal_default_seed20260928_A.replay](full_v1/0083_V3_portal_default_seed20260928_A.replay) |
| Default | 20260928 | B | 500 | [0084_V3_portal_default_seed20260928_B.replay](full_v1/0084_V3_portal_default_seed20260928_B.replay) |
| Portals | 20260928 | A | 500 | [0087_V3_portal_portals_seed20260928_A.replay](full_v1/0087_V3_portal_portals_seed20260928_A.replay) |
| Prisoners Dilemma | 20260928 | B | 255 | [0090_V3_portal_dilemma_seed20260928_B.replay](full_v1/0090_V3_portal_dilemma_seed20260928_B.replay) |
| Queen Of Spades | 20260928 | B | 330 | [0092_V3_portal_queen_of_spades_seed20260928_B.replay](full_v1/0092_V3_portal_queen_of_spades_seed20260928_B.replay) |
| Schooltime | 20260928 | A | 500 | [0093_V3_portal_schooltime_seed20260928_A.replay](full_v1/0093_V3_portal_schooltime_seed20260928_A.replay) |
| Schooltime | 20260928 | B | 500 | [0094_V3_portal_schooltime_seed20260928_B.replay](full_v1/0094_V3_portal_schooltime_seed20260928_B.replay) |
| Slithery Fight | 20260928 | A | 500 | [0095_V3_portal_slithery_fight_seed20260928_A.replay](full_v1/0095_V3_portal_slithery_fight_seed20260928_A.replay) |
| Slithery Fight | 20260928 | B | 500 | [0096_V3_portal_slithery_fight_seed20260928_B.replay](full_v1/0096_V3_portal_slithery_fight_seed20260928_B.replay) |
| Trauma | 20260928 | B | 500 | [0098_V3_portal_trauma_seed20260928_B.replay](full_v1/0098_V3_portal_trauma_seed20260928_B.replay) |
| Autarky | 20260929 | B | 500 | [0102_V3_portal_autarky_seed20260929_B.replay](full_v1/0102_V3_portal_autarky_seed20260929_B.replay) |
| Default | 20260929 | B | 500 | [0104_V3_portal_default_seed20260929_B.replay](full_v1/0104_V3_portal_default_seed20260929_B.replay) |
| Devil | 20260929 | A | 184 | [0105_V3_portal_devil_seed20260929_A.replay](full_v1/0105_V3_portal_devil_seed20260929_A.replay) |
| Prisoners Dilemma | 20260929 | A | 344 | [0109_V3_portal_dilemma_seed20260929_A.replay](full_v1/0109_V3_portal_dilemma_seed20260929_A.replay) |
| Prisoners Dilemma | 20260929 | B | 305 | [0110_V3_portal_dilemma_seed20260929_B.replay](full_v1/0110_V3_portal_dilemma_seed20260929_B.replay) |
| Queen Of Spades | 20260929 | A | 413 | [0111_V3_portal_queen_of_spades_seed20260929_A.replay](full_v1/0111_V3_portal_queen_of_spades_seed20260929_A.replay) |
| Queen Of Spades | 20260929 | B | 494 | [0112_V3_portal_queen_of_spades_seed20260929_B.replay](full_v1/0112_V3_portal_queen_of_spades_seed20260929_B.replay) |
| Schooltime | 20260929 | A | 500 | [0113_V3_portal_schooltime_seed20260929_A.replay](full_v1/0113_V3_portal_schooltime_seed20260929_A.replay) |
| Schooltime | 20260929 | B | 500 | [0114_V3_portal_schooltime_seed20260929_B.replay](full_v1/0114_V3_portal_schooltime_seed20260929_B.replay) |
| Slithery Fight | 20260929 | B | 500 | [0116_V3_portal_slithery_fight_seed20260929_B.replay](full_v1/0116_V3_portal_slithery_fight_seed20260929_B.replay) |
| Trauma | 20260929 | B | 500 | [0118_V3_portal_trauma_seed20260929_B.replay](full_v1/0118_V3_portal_trauma_seed20260929_B.replay) |
| Trophy | 20260929 | B | 193 | [0120_V3_portal_trophy_seed20260929_B.replay](full_v1/0120_V3_portal_trophy_seed20260929_B.replay) |

