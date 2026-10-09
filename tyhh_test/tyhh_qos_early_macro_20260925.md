# 早期宏观探索：遥测与新方案

本轮只改动 `tyhh_test`。探索遥测每 20 回合输出 `EXP_METRIC`，可在回放的 bot output 中查看；对战日志由 `benchmark.py --verbose` 保存。运行

```bash
python3 tyhh_test/analyze_exploration_metrics.py replays/<run目录>
```

可汇总双方仍存活龙的个体均值。

指标定义：`mean_unique_tiles` 是采样回合仍存活的每条龙，从出生以来见过的不同格数的均值；`mean_recent_repeat_percent` 是最近最多 64 个实际经过格中，落到本窗口内已经过格子的步数比例。它们是个体指标，不是全队视野并集；新出生的龙会拉低覆盖格数均值。第 200 回合已死亡的龙不进入样本，因此该时点存在存活者偏差。

## 版本与结果

| 版本 | 思路 | 对照基线战绩 |
|---|---|---:|
| `tyhh_qos_macro_metrics` | 当前 `tyhh_qos_explore`，只增加遥测 | 对手 |
| `tyhh_qos_fanout` | 大图早期加强远离可见队友龙头的分散收益 | 11/24 |
| `tyhh_qos_frontier_route` | 对四个方向估算可达未知前沿，提升路线目标权重 | 6/24 |
| `tyhh_qos_explore_v2` | 大图前 180 回合把扩张人口目标提高到最多 48 | 12/24 |

三项均在官方与仓库可用的 12 张地图上各打双方先手一局，对照均为 `tyhh_qos_macro_metrics`。基线胜场分别为 13/24、18/24、12/24。样本很小且先后手结果偏差明显，不应视为稳定胜率估计。当前没有新方案在全图对战上优于基线。

对照运行目录：

- fanout：`replays/20260925-183500-ojcg0rix`
- frontier route：`replays/20260925-182737-e72evf67`
- faster expansion：`replays/20260925-183949-7skightw`

## 指标参考

在完成路线采样修正后的 fanout 定向复测中，按存活龙均值汇总如下：

| 地图 | 回合 | 基线个体覆盖率 | 基线重访率 | fanout 个体覆盖率 | fanout 重访率 |
|---|---:|---:|---:|---:|---:|
| `big_empty` | 100 | 8.0% | 1.6% | 7.9% | 0.8% |
| `big_empty` | 200 | 7.5% | 3.6% | 7.8% | 3.2% |
| `schooltime` | 100 | 13.3% | 4.9% | 14.1% | 6.4% |
| `schooltime` | 200 | 9.7% | 2.5% | 17.8% | 14.6% |

数据目录：`replays/20260925-184732-rzr_e8rz`。第 200 回合 `schooltime` 的基线只有 11 条龙进入样本，fanout 为 43 条，受存活者差异影响很大。fanout 在 `big_empty` 略降重访率，在 `schooltime` 回合 100 的重访率反而上升；目前看不出一致改善。

分析脚本：`tyhh_test/analyze_exploration_metrics.py`。三个新版本都已编译；未将任何策略同步至 `tyhh`。
