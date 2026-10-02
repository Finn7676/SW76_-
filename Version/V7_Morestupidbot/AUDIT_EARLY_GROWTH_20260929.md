# 2026-09-29 前 70 轮发育分裂与珍珠优先级

本次修改范围为第 0–69 轮，第 70 轮起恢复既有策略。

| 长度与视野 | 新行为 |
| --- | --- |
| 长度至少 6 | 优先均分，6→3+3、8→4+4，不继续追珍珠或执行普通侦查任务 |
| 长度 4–5，有可一步安全吃到的珍珠 | 先移动吃珍珠，下一轮重新判断 |
| 长度 4–5，看见珍珠但无法一步安全吃到 | 均分，4→2+2、5→3+2 |
| 长度 4–5，没有看见珍珠 | 保留既有策略 |

均分的子龙长度为当前长度除以 2 后向下取整。新分支位于采集及普通侦查之前；既有开局受困救援、截图短死路的先吃珍珠再分出 3 节龙、即时避险和原有先手攻击判定仍优先执行。人口达到目标、分裂不合法、尾部位置未知或尾部出口封住时，继续原有行动选择。普通侦查任务在分裂后保留。

直线生长到 6 节时，尾巴通常已经离开视野。仅针对这项发育均分，使用身体历史中记录的尾部位置与已知出口，防止最终动作检查把分裂取消；没有尾部记录时不会按长度猜测尾巴。其他分裂的尾部检查保持原样。

此前已移除按 ID 分配“中心→反向→中心”的规则，继续按出生朝向选择路线。反向任务进入中心对称出生点的 3×3 区域后解除；中心任务分别完成横纵中轴穿越后解除。开阔传送门出口解除任务，小封闭区域探索返回后保留任务。BugNav、边界判定、既有任务截止规则保留。

验证结果：10 组 C++ 回归全部通过；7 个协议回放及新增发育回归通过 ASan/UBSan。覆盖第 69/70 轮切换、4–5 节一步吃珠与远珠均分、吃到 6 节后分裂、尾部离开视野、未知/封闭尾部、人口上限、侦查任务保留和即时敌人威胁。原先长度 8 追珍珠的协议回放预期按新要求更新为 `SPLIT 4`。

`dilemma`、`devil`、`big_empty` 各换边运行到第 80 轮，共 6 场 WASM 对局。执行错误、能够合法分裂却无动作、自撞均为 0；最高单回合 CPU points 为 6,932,103。对局仍存在战斗死亡和受困小龙无动作死亡。这些检查验证执行和行为，没有做胜率或发育速度的对照实验。

截图结构重建的两个角落场景（初始长度 3、4）仍执行 `MOVE W → MOVE N → SPLIT 3`，子龙出生当轮行动并继续逃出；留在死路中的母龙随后死亡。该场景不是原截图整局的精确回放。

复现：

```sh
python3 verification/run_checks.py
python3 verification/protocol_regressions.py --sanitize
c++ -std=c++20 -O1 -g -fsanitize=address,undefined -x c++ -I. verification/early_growth.cpp.txt -o /tmp/v4-early-growth
/tmp/v4-early-growth
python3 verification/short_escape_engine.py
V4_AUDIT_OUT=/tmp/v4-growth-check python3 verification/audit_matches.py growth . ../tyhh wasm 80 dilemma devil big_empty
```

修改文件：`planner.hpp`、`main.cpp`、`verification/protocol_regressions.py`；新增 `verification/early_growth.cpp.txt`。机器可读结果：`verification/early_growth_results.json`。此前开局策略报告与结果为历史记录，未覆盖。

源码指纹（按文件名排序，文件名与内容分别以 NUL 分隔，SHA-256）：`d65c31acef6c38edec40d1f65a7d4efad20179d0b9f1c2c03ed3c6b1505e357b`。源码包包含当前 C++ 源码、头文件、bot.toml 和本说明，不含编译缓存；验证脚本留在原项目。
