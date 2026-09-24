# Version04 基座迭代报告

## 为什么旧路线打不过 Version04

此前路线从旧 `tyhh/main.cpp` 出发，只叠加探索、密度和旗舰评分，遗漏了 Version04 已经验证的综合能力：有界 BFS 路由、直接观察/记忆/声呐分层、弯曲身体重建、多步攻击搜索、短龙条件兑子、空间 pocket 判定、后期分裂策略和传送门保守处理。结果是局部规则变多，但核心状态估计和战术顺序反而弱于 Version04，尤其在 `big_empty` 和 `help` 的高人口碰撞场景中更明显。

## 新路线

新实验全部从 `Version/Version04` 复制到 `tyhh_test/tyhh_v04_plus_m*`，只增加单个 tyhh 机制。旧版 `tyhh` 和 Version04 都作为对照，未修改主目录。

| 版本 | 单变量改动 | 结果与决策 |
|---|---|---|
| M0 | Version04 原版 | 基线 |
| M1 | 拥挤时降低珍珠吸引、提高 pocket 开放度；大图 ID 分区方向奖励 | 对旧 tyhh 四图 7/8；对 Version04 四图 4/8，保留 |
| M2 | 大图探索龙优先前沿 | 对 Version04 压力图 0/4，拒绝 |
| M3 | 64×64 且单位数达到 4 后停止主动分裂 | 对 Version04 四图 3/8；对旧 tyhh 四图 7/8，作为候选 |
| M4 | 去掉 Version04 的 ID 微小平局奖励 | 对 Version04 四图 3/8，未改善阵营偏差 |
| M5 | 64×64 压力图进一步降低珍珠权重、提高开放空间权重 | 对 Version04 `big_empty/help` 3/4；对旧 tyhh 四图 5/8，当前最强候选 |
| M6/M7 | 对 Team B 增加方向补偿 | 仅改变阵营方向，不能证明算法提升，拒绝 |

M5 代码位于 [tyhh_v04_plus_m5](/home/tyhh/SW76_-/tyhh_test/tyhh_v04_plus_m5)，严格编译通过：

```bash
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror \
  tyhh_test/tyhh_v04_plus_m5/main.cpp -o /tmp/v04m5
```

## M5 对旧版 tyhh

四张关键地图各交换一次先手，共 8 局：

| 地图 | M5 先手 | 旧 tyhh 先手 |
|---|---|---|
| arena | M5 胜 | 旧 tyhh 胜 |
| default | M5 胜 | 旧 tyhh 胜 |
| big_empty | M5 胜 | M5 胜 |
| help | M5 胜 | M5 胜 |

合计 M5 `5/8`。其中 `big_empty`、`help` 双方向都胜，说明 Version04 基座加密度控制确实解决了旧路线最严重的压力图问题。

## M5 对 Version04

四张关键地图各交换一次先手，共 8 局：

| 地图 | M5 先手 | Version04 先手 |
|---|---|---|
| arena | Version04 胜 | Version04 胜 |
| default | Version04 胜 | Version04 胜 |
| big_empty | M5 胜 | M5 胜 |
| help | M5 胜 | Version04 胜 |

合计 M5 `3/8`。M5 已在 `big_empty` 双方向击败 Version04，并在 `help` 的一个方向获胜，但尚未同时击败 Version04 的全部四图。

## 阵营偏差与解释边界

Version04 自身对打时，在 `arena/default/help` 有明显 Team B 优势，在 `big_empty` 有 Team A 优势。因此单独看 Team A 或 Team B 的一局会把先手偏差误判为策略收益。所有结论均按交换先手汇报；当前 M5 的 3/8 和 5/8 是开发样本，不是稳定胜率。

## 当前结论

可以确认的结论是：以 Version04 为基座，再加入 tyhh 的拥挤空间控制，明显比从旧 tyhh 直接叠加规则更合理；M5 已达到“在关键压力图打过 Version04 和旧版 tyhh”的阶段性目标。

尚未实现的目标是“所有场景同时打过 Version04 和旧版 tyhh”。当前不应把 M5 同步到主 `tyhh`，因为它在 `arena/default` 对 Version04 仍然退后。

下一步应围绕 Version04 的战术决策顺序做实验：保持 M5 的压力图权重，单独比较 `short_trade`、`find_dive`、分裂时序和 `score_step` 的组合；每次只改一个机制，并至少用双方各 10 局验证，避免继续用单局方向补偿掩盖阵营偏差。
