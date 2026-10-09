# Queen of Spades 传送门实验

本轮实验只修改 `tyhh_test`，没有同步到 `tyhh`。基线是当前 `tyhh`，对手固定为 `tyhh_test/xxqz`。每个版本在 `maps_BeforeSprinter` 的 9 张地图上各进行双方先手各一局，共 18 局。

## 实验版本

| 版本 | 修改 |
|---|---|
| `tyhh_qos_anchor` | 检测到传送门附近的珍珠区域后，抑制母龙离开，并仅在尾部存在安全出口时分裂 |
| `tyhh_qos_explore` | 前 80 回合提高传送门移动得分，并允许长度为 2 的龙进入传送门 |
| `tyhh_qos_combo` | 同时启用上述两项修改 |

源码目录：

- `tyhh_test/tyhh_qos_anchor/`
- `tyhh_test/tyhh_qos_explore/`
- `tyhh_test/tyhh_qos_combo/`

## 全场景结果

| 版本 | 胜场 | 胜率 | queen_of_spades | 相对当前 `tyhh` |
|---|---:|---:|---:|---:|
| 当前 `tyhh` 基线 | 11/18 | 61.1% | 0/2 | 基线 |
| `tyhh_qos_anchor` | 5/18 | 27.8% | 0/2 | -6 胜 |
| `tyhh_qos_explore` | 9/18 | 50.0% | 1/2 | -2 胜 |
| `tyhh_qos_combo` | 5/18 | 27.8% | 1/2 | -6 胜 |

逐图胜场：

| 地图 | 基线 | anchor | explore | combo |
|---|---:|---:|---:|---:|
| `Colosseum` | 2 | 0 | 2 | 0 |
| `arena` | 1 | 1 | 1 | 1 |
| `big_empty` | 1 | 1 | 1 | 1 |
| `default` | 2 | 1 | 2 | 1 |
| `default_small` | 0 | 0 | 0 | 0 |
| `help` | 2 | 1 | 0 | 0 |
| `queen_of_spades` | 0 | 0 | 1 | 1 |
| `schooltime` | 0 | 0 | 0 | 0 |
| `trophy` | 2 | 1 | 2 | 1 |

## Queen of Spades 回放

- 基线：`replays/20260924-160857-oqm02xnf/0013_queen_of_spades_bot1-side-A.replay`（负局）
- anchor A 先手：`replays/20260924-170339-t7d_4gdf/0013_queen_of_spades_bot1-side-A.replay`（负局）
- anchor B 先手：`replays/20260924-170339-t7d_4gdf/0014_queen_of_spades_bot1-side-B.replay`（负局）
- explore A 先手：`replays/20260924-170729-az3k5n21/0013_queen_of_spades_bot1-side-A.replay`（负局）
- explore B 先手：`replays/20260924-170729-az3k5n21/0014_queen_of_spades_bot1-side-B.replay`（胜局）
- combo A 先手：`replays/20260924-171110-p_tgyp8f/0013_queen_of_spades_bot1-side-A.replay`（胜局）
- combo B 先手：`replays/20260924-171110-p_tgyp8f/0014_queen_of_spades_bot1-side-B.replay`（负局）

## 结论

本轮结果不支持把“传送门内固定保留母龙”的规则直接并入主版本。anchor 和 combo 的全场景胜率明显下降，说明当前锚定条件过宽，影响了普通地图中的分裂和移动节奏。

提高早期传送门探索积极性在 `queen_of_spades` 上从 0/2 提升到 1/2，但全场景仍为 9/18，低于基线 11/18；它可以作为后续针对该地图的候选策略，不能作为全局默认策略。

本轮每个地图只有两个先手样本，结论属于开发回归描述，不是稳定胜率估计。

## 后续占用锁修改

随后在 `tyhh_qos_explore` 中加入了传送门房间占用机制：进入房间的龙记录 `portal_id`，通过四个方向的 sonar 广播 `M_PORTAL_OCCUPIED`，其他龙在占用状态有效时禁止进入；占用状态连续 6 回合未刷新后自动过期。当前房间内的龙仍可以离开。

最初的占用锁实现误用了单参数 `send_sonar(msg)`，该接口只沿当前朝向发送，不能视为全队广播；同时早期传送门奖励只是普通评分加分，不能保证实际进入。两处已修正：占用消息改为四向发送，早期阶段发现未占用传送门时在分裂和普通评分前直接进入。

随后发现实验文件顶部仍有 `#define FEAT_SONAR 0`，导致占用广播区块完全未编译；该配置已改为 `FEAT_SONAR 1`。启用真实广播后的完整回归结果为 8/18（44.4%），但 `queen_of_spades` 提升为 2/2。结果文件：`replays/20260924-174734-5tmhcadq/results.csv`。这证明之前观察到的占用锁行为并不可信，当前结果才是启用广播后的有效回归。

在此基础上又加入了两项不依赖地图名称的运行时规则：短龙禁止移动到己方更长龙的任意可见身体段；当运行时地图面积较大且处于前 220 回合时，提高未探索格收益、提高远离己方龙头的收益并加重局部拥挤惩罚。最新完整回归为 10/18（55.6%），`queen_of_spades` 为 2/2，`schooltime` 为 1/2，结果文件：`replays/20260924-181158-4xa96q2q/results.csv`。

本轮进一步增加了运行时前沿方向评分：对四向候选分别计算可达未探索前沿数量，不使用地图名称或固定坐标。并建立 `tyhh_qos_explore_hunt`，在此基础上让长度不超过 5 的短龙向可见的明显更长龙靠近，但避免直接进入敌方龙头相邻格。

| 版本 | 对 `xxqz` | 对当前 `tyhh` |
|---|---:|---:|
| `tyhh_qos_explore` | 12/18（66.7%） | 10/18（55.6%） |
| `tyhh_qos_explore_hunt` | 10/18（55.6%） | 9/18（50.0%） |

结果目录分别为：`replays/20260924-183859-98dlelby`、`replays/20260924-183859-g0qbrn6u`、`replays/20260924-183859-5d9b9b8_`、`replays/20260924-183859-9chzn6ff`。在本轮样本中，前沿分散版本优于 hunt 版本，hunt 版本暂不建议合并。
