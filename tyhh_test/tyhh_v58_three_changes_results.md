# v58 三处改动首轮结果

固定对照为 `tyhh_test/brawl_v58`，命令为 `benchmark.py <候选> tyhh_test/brawl_v58 --maps all --games 2`。每张图各运行 A/B 两局，共 18 局；胜率是候选作为 bot1 的描述性比例。

| 版本 | 胜负 | 胜率 | 主要地图切片 |
|---|---:|---:|---|
| `tyhh` 主目录历史运行 | 3/18（`20260923-070359-79nx2imd`） | 16.7% | 这是主目录对 `brawl_v58` 的历史比较，仅作背景；本轮没有把它当作新的基线自对局 |
| `tyhh_v58_head` | 5/18 | 27.8% | Colosseum 50%，big_empty 100%，arena/help/queen_of_spades/schooltime/trophy 0% |
| `tyhh_v58_territory` | 10/18 | 55.6% | help 100%，schooltime 100%，arena/big_empty/default/Colosseum/queen_of_spades/trophy 各 50%，default_small 0% |
| `tyhh_v58_frontier` | 5/18 | 27.8% | schooltime 100%，arena/big_empty/default_small 各 50%，Colosseum/default/help/queen_of_spades/trophy 0% |
| `tyhh_v58_combo` | 4/18 | 22.2% | big_empty/default/schooltime/trophy 各 50%，其余地图 0% |

## 判定

`territory` 是首轮唯一呈现全局改善信号的单项候选，尤其在 `help` 和 `schooltime` 上改善明显；`head` 与 `frontier` 未显示稳定收益。组合版本为 4/18，低于 territory 单项，不纳入后续扩大局数的候选。

所有版本均成功编译并完成 18 局，回放完整保留。对应结果目录：

- `replays/20260923-181655-14u6y480`（head）
- `replays/20260923-182050-xpdiyx7v`（territory）
- `replays/20260923-182512-4g2hbokh`（frontier）
- `replays/20260923-182903-usovimv5`（combo）

`results.csv` 记录地图、阵营、胜负和回放/日志路径；回合数可由同目录 `.log` 中的 `after N rounds` 提取。由于每图仅 2 局，这些比例仅作筛选依据，不作稳定胜率结论。
