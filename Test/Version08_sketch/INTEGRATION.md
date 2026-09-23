# tyhh / sonar2 能力接入说明

本版以 xxqz-c47v1 为主线，保留其 View / StepInfo / Memory / Pick 接口和多步攻击搜索，吸收另外两个 bot 的可复用能力。

## 原 bot 的优点与接入方式

| 来源 | 优点 | 接入结果 |
|---|---|---|
| tyhh | 长度 2 的龙只在有其他队友、敌方可见长度更大时相邻兑子 | `tactics.hpp::short_trade`，放在原攻击搜索之前，禁止隔墙兑子和最后一条龙自杀 |
| tyhh | 识别敌方头部附近和朝向前方的风险 | `tactics.hpp::tactical_score`，使用 View 快照；考虑海带阻隔，仅针对敌方，队友仍走原有拥挤评分 |
| tyhh | 优先可见珍珠，再考虑记忆和生成点，局部 BFS 绕障碍 | 与 sonar2 的有界 BFS 合并为 `navigation.hpp::find_forage_route` |
| tyhh | 身体模拟、核心长龙保护 | 复用 xxqz 已有的身体重建、多步攻击模拟和长度保护；不移植存在弯曲身体重建问题的旧模拟器 |
| sonar2 | 有界 BFS 返回第一步，能规划视野外的采集与探索 | 单次 BFS 默认深度 24、最多发现 320 个节点；优先级分层，第一步限制在 xxqz 已选安全等级内 |
| sonar2 | 队友共享珍珠目击，扩大资源信息覆盖 | `sonar.hpp`：带时间戳的新协议，过期清理、当前观察优先、仅广播直接看见且未准备吃掉的珍珠 |
| sonar2 | 小图对受困敌头施压，大图避险 | 合并到 `tactical_score`；仅有队友的短龙在小图围堵，未知传送门出口仍算可能逃生通道 |
| sonar2 | 空间容量与拥挤评分 | 复用 xxqz 的 pocket、hard/soft trap 分级及 ally_heads 评分，避免重复计权 |
| sonar2 | 后期停止主动分裂，保留增长 | xxqz 已在 round >350 停止主动分裂，比 sonar2 的大图 round >=440 更早；保持现有政策和紧急分裂 |

不直接接入 tyhh 的传送门移动：其常规候选本身也排除了 portal；三个原版都没有可直接复用且验证完整的通用传送门移动策略。

## 接口与调用顺序

- `StrategyConfig` 位于 `strategy.hpp`。routing、sonar、head_risk、corner_pressure、short_trade 五个开关默认开启，可分别关闭做消融。
- `Memory::observe(ct, game, config)` 保存直接观察时间，生成倒计时与已有珍珠分开过期。直接珍珠记忆保留最多 32 回合，声呐报告最多 8 回合。
- `receive_sonar(ct, game, memory, config)` 仅接收新协议，拒绝未来、越界、过期和被更新的本地观察否定的消息。消息抵达时间不刷新观测年龄。
- `find_forage_route(view, memory, allowed_first, config)` 返回 `Route{goal, first, distance, kind}`。目标顺序：可见珍珠、直接记忆珍珠、声呐珍珠、即将生成点、探索边界。同层按 BFS 最短路与固定方向顺序选择。
- `tactical_score(ct, view, step, config)` 返回附加评分。它不会放行不安全候选，也不会更改原有 hard/soft trap 分级。
- `short_trade(ct, view, config)` 返回可选的相邻攻击方向。
- `choose(ct, view, memory, config)` 返回原有 Pick，增加 routed 标记；搜索攻击目标时保留原 stalk 逻辑。
- `act(ct, game, config)`：观察/接收 → 相邻短龙兑子 → 原多步攻击 → 原分裂政策 → 寻路与风险评分 → 输出/广播 → 兜底。

声呐 32 位编码：高 8 位签名 0xA7，3 位保留位，9 位绝对 round，6 位 x，6 位 y。覆盖 500 round、64×64 地图。只传播亲眼所见，不转发消息；原 sonar2 的裸坐标协议不兼容。团队统一运行本版时使用新协议。

未知地形只用于规划假设，执行仍限制为当前可见、无占用、EMPTY 边的安全第一步。常规寻路不跨传送门。声呐开关关闭后，不发送、不接收、不把共享报告加入寻路目标。routing 关闭则恢复原距离式目标选择；记忆修复独立保留，因此关闭所有新策略不等于字节级原版。

## 验证

在本目录运行：

```sh
python3 tests/run.py --sanitize
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror main.cpp -o /tmp/xxqz-integrated
```

49 项行为检查覆盖绕障碍、不可达资源、搜索预算、安全第一步约束、环面边界、弯曲身体、声呐编码边界与过期、可见空格优先、旧报告不复活、兑子限制、墙后敌头、长短龙区别，以及 act 输出和广播。通过 AddressSanitizer、UndefinedBehaviorSanitizer 和严格编译警告检查。

使用本机 unswbc，将改版与接入前的 xxqz 原版在四张 maps_BeforeSprinter 地图各交换 A/B 一次：

| 地图 | 改版 A | 改版 B |
|---|---|---|
| arena | 胜，51 round | 负，130 round |
| default | 胜，500 round | 胜，500 round |
| big_empty | 胜，500 round | 负，500 round |
| schooltime | 负，500 round | 胜，500 round |

合计 5 胜 3 负，没有观察到进程退出、超时、非法动作或协议警告。详细结果见 tests/validation.json。这是小样本运行回归，不是稳定胜率结论，也没有证明每个新增模块单独提升胜率。C++ 使用本机 runner，尚未验证赛事环境的计算点数预算。

## 历史阶段：V05 instrumentation

`Version05_sketch` 已完成交接文档的 V05（基线与测量）阶段。增加可选逐回合工作量统计、独立编译开关、可重复的双边基准和行为等价验证；未修改策略优先级、评分、分裂条件或搜索预算。Version04 保持冻结。

详细接口、计数定义、使用命令、依赖关系审计与本次结果见 [V05.md](V05.md)。`tests/v05-validation.json` 记录 V05 当时的验证；上文的 5 胜 3 负及其他 validation 文件属于此前策略合并的历史记录。

默认编译不启用统计。使用 `-DXXQZ_ENABLE_STATS=1` 编译后，每个龙回合输出一条 `LOG V05_STATS`。基准脚本 `tools/benchmark_v05.py` 同时检查默认/统计构建、交换双方、逐回合动作序列、运行失败和计数覆盖。

## 历史阶段：V06 Unified Perception

依据 `CODEX_HANDOFF_UNSW_BATTLECODE_V2.md`，工作目录名仍为 `Version05_sketch`，代码阶段已升级到 **V06**。V05 原版冻结到同级 `Version05_frozen`；Version04 保持不变。

默认由 `sense_nearby(ct, game, memory, config)` 完成统一感知：旧记忆老化 → 单次原始可见格扫描（一次解码，同时更新 Memory、View 和候选列表）→ 接收并校验声呐 → 身体重建、敌人分组、安全候选。广播使用 `Perception::visible_pearls`，不再扫描所有可见格；该列表仅包含未占用的直接可见珍珠，移动目的地在广播时排除。

策略顺序、分裂条件、寻路预算和战斗评分保持原样。`unified_perception` 默认开启，可通过 `StrategyConfig` 或 `-DXXQZ_UNIFIED_PERCEPTION=0` 关闭作对照。统计增加 `perception_tiles_scanned`，保留 `LOG V05_STATS` 格式以兼容原解析器。这里的 V05 是统计格式标签，不是当前策略版本。

审计、接口、流程图、测试命令和测量结果见 [V06.md](V06.md)，V06 当时的验证见 `tests/v06-validation.json`。该段记录 V06 完成时的状态。

## 历史阶段：V07 Derived Perception Cache

当前工作目录为 **Version07_sketch**，原 V06 冻结到同级 `Version06_frozen`。以前存放 V06 的 `Version05_sketch` 保持不变，后续开发请使用 Version07_sketch。

新增拓扑缓存与威胁缓存：同一可执行第一步所能进入的连通区域只计算一次，候选直接查询区域大小、未知边界和内部度数；首次战术评分时计算敌头可达/朝向掩码与逃生信息，同一回合后续候选复用。两项独立开关 `component_cache`、`threat_cache` 默认开启，可分别关闭做消融。对于不对称自由格边，保留旧的有向洪泛行为。

V06 的单次原始视野扫描、记忆/声呐优先级和全部原策略保持不变。详见 [V07.md](V07.md) 的流程、接口、缓存生命周期与测量结果；当前验证为 `tests/v07-validation.json`。该段记录 V07 完成时的状态。

## 当前阶段：V08 Stateful Expansion + Navigation

当前目录为 **Version08_sketch**，上一版冻结为同级 `Version07_frozen`，原 `Version07_sketch` 保持不变。新增每条龙的导航状态、早期方向分散、信息增益/队友覆盖重叠评分、目标保持与失效检查、两步/四步循环识别、临时避开重复首步、目标冷却及局部逃生恢复。

V08 是策略变更，不要求默认行为与 V07 一致。近距离可见珍珠、攻击/分裂优先级和现有安全候选分级保留；扩张优先只用于较大地图、早期、无可见敌头且覆盖不足的部分龙。全部 V08 开关关闭时需与 V07 完全等价。未加入 BugNav、对称推断、V09 MoveEval 或 V10 角色系统。

详见 [V08.md](V08.md) 的流程、参数、功能开关及验证结果；本版验证为 `tests/v08-validation.json`。
