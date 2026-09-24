# 初步对战评估

测试日期：2026-09-21

对手：`SW_Champion`

配置：`arena`、`Colosseum`、`default_small`，每张地图 4 局，双方各作为 Team A 和 Team B 两局。

## 结果

| 项目 | 结果 |
|---|---:|
| 总局数 | 12 |
| `tyhh_rl` 胜 | 2 |
| `SW_Champion` 胜 | 10 |
| 平局 | 0 |
| 总胜率 | 16.7% |

分地图胜率：

| 地图 | `tyhh_rl` 胜率 |
|---|---:|
| `arena` | 50.0% |
| `Colosseum` | 0.0% |
| `default_small` | 0.0% |

分阵营胜率：

| 阵营 | `tyhh_rl` 胜率 |
|---|---:|
| Team A | 33.3% |
| Team B | 0.0% |

## 解释

这是小样本初筛，不足以证明一般化性能。结果显示当前固定线性权重明显不如 `SW_Champion`，并且存在较强的阵营差异。下一步应检查 Team B 的行动顺序、目标评分、传送门目标偏好和分裂策略，再进行权重训练；不应直接把当前版本提交线上。

原始回放和 CSV 位于：

```text
replays/20260921-180906-yrkkov44/
```

## 参数调整后的复测

将 `maxPopulation` 从 3 调整为 8、`reserveSplitLength` 从 18 调整为 10，并保持安全分裂条件不变。

### 对 `tyhh_test/tyhh_core`

6 局：`tyhh_rl` 胜 2 局，`tyhh_core` 胜 4 局，胜率 33.3%。地图结果为 `arena` 50%、`Colosseum` 0%、`default_small` 50%。相较调整前没有明显改善。

### 对 `SW_Champion`

6 局：`tyhh_rl` 胜 3 局，`SW_Champion` 胜 3 局，胜率 50.0%。地图结果为 `arena` 100%、`Colosseum` 50%、`default_small` 0%。

该结果仍是小样本，不能视为稳定优势；但说明增加受控分裂人口上限后，原先对 `SW_Champion` 的 16.7% 初步胜率有所改善。`default_small` 仍是主要弱点，下一步应针对小地图的拥挤、分裂损失和旗舰保护继续调参。
