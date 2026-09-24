# v58 三处决策改动验证方案

## 目标

以 `tyhh_test/brawl_v58` 为固定基线，在 `tyhh_test` 中逐项验证三项机制改动。所有候选目录均以 `tyhh` 开头，基线目录不修改。

## 固定评测协议

- 地图：`arena`、`big_empty`、`default`、`default_small`、`help`、`queen_of_spades`、`schooltime`、`trophy`、`Colosseum`。
- 每个版本每张地图运行 2 局，分别让候选作为 Team A 和 Team B。
- 首轮用于筛选；每项只改变一个机制，保留 v58 其他逻辑。
- 记录 `results.csv`、地图、阵营、胜负、回合数和 replay 路径。2 局/图只报告描述统计。
- 运行命令：

```bash
mkdir -p /tmp/clang-toolchain
ln -sf /usr/bin/clang++ /tmp/clang-toolchain/c++
PATH=/tmp/clang-toolchain:$PATH python3 benchmark.py <candidate> tyhh_test/brawl_v58 --maps all --games 2
```

## 候选一：限制不划算的撞头

目录：`tyhh_test/tyhh_v58_head/`。

在 v58 的相邻敌头处理处加入 `should_trade_head`：

1. 敌方可见长度必须严格大于己方长度，至少多 2 节；
2. 己方存活龙数量必须大于 1；
3. 己方当前龙长度必须不超过 5，作为可牺牲短龙；
4. 己方长度更长、相等或全队只剩一条龙时禁止主动撞头；
5. 其他移动评分保持 v58 原值。

预期：减少等长交换和长龙误杀，提高终局最长龙存活率。证伪信号：小图淘汰速度明显下降或总胜率下降。

## 候选二：障碍感知领地估值

目录：`tyhh_test/tyhh_v58_territory/`。

替换 `controlled_space`：

1. 从每个可见敌方头部做多源 BFS，边只接受 `path_edge_open` 为真的边；
2. 从候选目的地做己方 BFS；
3. 对每个己方可达格，只有己方距离小于或等于敌方距离时计入控制区；
4. 敌方头部不可见时，敌方距离设为无穷，退化为候选点的安全可达空间；
5. 未知区域不主动计入控制区，避免把未知边当作已确认可通行边。

预期：在 `arena`、`help`、`queen_of_spades` 等有障碍地图减少错误围堵和死路选择。证伪信号：大图因估值过于保守而探索覆盖下降。

## 候选三：约束未知区域寻路

目录：`tyhh_test/tyhh_v58_frontier/`。

分离珍珠路径和探索路径：

1. 珍珠目标 BFS 只沿当前已观测且确认 `EMPTY` 的边传播；
2. 记忆珍珠必须经过已知路径可达性检查；
3. 探索目标只选择 `g_seen` 边界的未知邻格；
4. 执行 `planDir` 前再次调用 `safe_step(here, planDir)`，首步无法由当前视野确认安全时放弃路径；
5. 放弃路径后按 `flood_space`、开放邻居数和当前可见珍珠选择安全动作。

预期：降低未知边、未知海带和错误记忆目标造成的死亡。证伪信号：探索速度和资源获取下降，导致 500 回合长度不足。

## 组合候选

目录：`tyhh_test/tyhh_v58_combo/`。只有当三项单项候选均能编译并完成首轮评测后，才合并三项改动。组合结果仍需与固定 v58 在相同协议下比较。

## 验收标准

- 每个候选成功编译并完成全部计划对局；
- 单项候选相对 v58 的胜负和地图切片结果落盘；
- 任何未提升的假设保留为失败结果，不删除 replay；
- 只有在至少一个候选在全图和主要失败地图均显示改善后，才考虑继续扩大局数；不自动覆盖主 `tyhh`。
