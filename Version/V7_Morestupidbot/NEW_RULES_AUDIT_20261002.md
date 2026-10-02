当前补充策略已记录于 [QUEEN_POLICY_20261002.md](QUEEN_POLICY_20261002.md)，包括早期分裂、Queen + 2 刺杀门槛与普通龙救援自杀。下文保留第一阶段的核查记录。

# V5 新规则检查与第一阶段修正（2026-10-02）

本次范围：更新物理移动模拟；接入固定 Queen 身份和基础保护；检查其他策略与新规则的冲突。检查对象是当前 V5 工作目录，包括已有未提交的密度、导航、声呐和喂食修改。没有覆盖这些修改，也没有沿用旧 Git HEAD 重新生成代码。

规则依据：[胜负与行动顺序](https://game.battlecode.au/docs/structure)、[移动结算](https://game.battlecode.au/docs/execution-order)、[移动费用](https://game.battlecode.au/docs/movement)、[CLI 引擎版本](https://game.battlecode.au/docs/cli)。初次核查时本机为 1.1.0，使用隔离的官方 1.2.3 验证；随后按用户要求已将本机工具包升级为 1.2.3。

## 已修正

| 问题 | 处理 | 代码位置 |
|---|---|---|
| 所有 sprint 都假设只免费一步 | 统一 MovementRules：免费 ceil(行动开始长度/4)，途中增长不能增加本次免费额度 | core.hpp:211 |
| 碰撞前先移除付费尾段，可能误放行进入当前尾巴 | 先检查能否支付，再检查完整身体的碰撞，移动/吃珠后才扣额外尾段 | sprint.hpp:43 |
| 撞头收益使用本步提前扣费后的长度 | 碰撞步不发生移动、吃珠或扣长度，用上一步结束时的实际身体评估 | sprint.hpp / planner.hpp:sprintRam |
| 敌人攻击模拟也按旧免费额度结算 | 更新 SprintModel 和 planner 的逐步攻击预测；同时修正旧身体格释放判定 | sprint.hpp:buildReach / planner.hpp:enemyAttackRisk |
| 把八步搜索上限误作敌方真正移动上限 | 达到八步或状态预算但仍有支付能力时，扩展保守地形区域；未知长度按地图范围估计 | sprint.hpp:97 |
| 逃跑固定最多三步，浪费免费移动 | 上限改为 min(8, ceil(初始长度/4)+2)，仍最多付费两段，搜索状态上限 1,024 | sprint.hpp:198 |
| 声呐行动后长度、身体和 portal 占用预测仍扣 steps-1 | 使用同一 MovementRules，按实际免费额度预测尾部、剩余长度与报告 | sonar.hpp:300 |
| Queen 被当成普通龙，ID 1 尤其没有旧 ID%8 的保护 | 显式 queen 标记和 TurnState::isQueen，ID 0/1 从第 0 回合保护；不依赖 CH_START、长度或 FEAT_CHAMPION | core.hpp:210 / roles.hpp:29 |
| Queen 可以主动撞头、等长兑子、晚期献祭 | 排除普通攻击/喂食分支；最终动作检查也拒绝已知敌头碰撞和显式献祭 | donation.hpp:33 / planner.hpp |
| Queen 在成长/扩张或 pearl-pocket 策略中把长度转给子龙 | 第 0–149 回合允许安全平均分裂；之后禁止主动成长分裂，仍禁止尾部接管的死路采集 | planner.hpp:earlyGrowthAction / expansion / tacticalSplit |
| 门口 HOLD 可以命令 Queen 自杀或留头两段 | Queen 不执行这两项门口处理；无路时仍尝试正常物理逃生/救援 | planner.hpp:portalHoldEmergency |
| Queen 风险评分只防范短至少三段的敌人 | Queen 纳入所有敌方长度；普通龙保留原来的相对长度偏好，关闭该偏好也不关闭 Queen 保护 | planner.hpp:1890 |
| 救援分裂把 Queen 几乎全部长度移给子龙 | Queen 紧急救援只分出两段，原 ID 和尽可能多的长度留在本体；新子龙不会继承 Queen 标记 | planner.hpp:recoverySplit / sprintEscape |

初次修改采用 Queen 不主动分裂的保守策略；现已按用户要求恢复第 0–149 回合的发育分裂，详见 [Queen 策略更新](QUEEN_POLICY_20261002.md)。这不是引擎禁止 Queen 分裂。被自身/地形困住时仍允许 SPLIT 2，无法保证这种救援随后一定救活 Queen。最后没有任何合法动作时，引擎仍会杀死它。

## 已确认仍需后续策略改造

这些不违反动作合法性，但目标函数或启发式仍来自旧游戏。用户本次先修改第 1、2 项，因此记录下来，未改成完整 Queen 战术系统。

| 优先级 | 当前行为与新模式的差异 | 后续建议 |
|---|---|---|
| 高 | 已加入独立 Queen 刺杀、+2 门槛、400 回合放宽和三回合追击情报；尚未按双方 Queen 存活/比分决定追击价值 | 后续补全死亡确认与胜负状态 |
| 高 | 普通龙喂食仍要求接收者至少长两段，再按长度/距离排序；较短的己方 Queen 可能拿不到资源 | 对 Queen 单独评估供给收益和接收安全；双方 Queen 已死时才能明确切回最长龙目标 |
| 高 | 声呐能报告 ID 和长度，但没有完整的远端 Queen 存活、最终长度差和两 Queen 已死的局面状态 | 设计带时效、可信度和死亡确认的 Queen 情报；不能把看不见 Queen 当作 Queen 已死 |
| 中 | 普通龙前 70 回合平均分裂及常规剥两段，不计 5/9/13/17 等速度档位损失 | 把分裂后免费步数和团队长期收益纳入决策 |
| 中 | 大多数采集、探索、密度疏散仍一次只移动一步；争珠 sprint 只搜两珠、两步 | 逐步加入免费多步路径，保留身体模拟、可见性和队友保护 |
| 中 | controlledSpace 仍以格数比较双方到达时间；收珠/claim ETA 和喂食拾取窗口按普通单步回合估算 | 按免费速度档位估算到达回合，不能直接把格数当回合数 |
| 中 | 队友意图主要报告下一步、堵出口保护主要考虑本回合尚未行动的更高 ID；普通龙没有给更早行动的 Queen 保留下一回合完整路线 | 扩展行动后占用和 Queen 通路协调 |
| 中 | Queen 保留原有开局方向、中心线与普通资源探索行为，仍可能进入未知 portal | 单独设计 Queen 资源区选择和已知安全通路 |
| 中 | Action 容量仍为 8；完整攻击搜索 4 步；逃跑受 8 步/1,024 状态限制 | 更长的龙可能有超过 8 个免费步；后续按收益与 CPU 预算扩大动作容量和搜索 |
| 中 | 威胁预测仍依赖当前观察、其他龙当前身体、已知 portal 和有限未知 pearl 估计 | 不把“模型可达区域外”当存活证明；后续加入候选动作后的世界变化 |

密度区域、最低长度 2、合法分裂检查、尾部反转、墙/portal/环绕、每一步吃珠、死亡掉落、按 ID 执行、64-bit 声呐和协议输入没有发现由这次公告直接引入的协议冲突。新版官方 helper 主要改变费用注释和 end_turn 的协议声明；项目已经自行输出 PROTOCOL 3，不能直接覆盖 helper 丢失现有 sonar_protocol3 字段。

## 验证与工具限制

- 17 组 C++ 回归，包含新增 queen_rules；Queen 单独测试在 FEAT_CHAMPION=0、FEAT_V3_SHORT_AVOID=0 下也通过。
- 7 组历史协议输入以 AddressSanitizer/UndefinedBehaviorSanitizer 运行，无内存诊断。devil_released_pearl_priority 的末动作现在为 MOVE N；未知敌长扩大危险区域后，原 MOVE NE 的终点不再被判定为区域外。
- 官方 1.2.3 引擎：134 个场景，覆盖长度 2/3/4/5/8/9/12/13/16/17/20/21/24、至多八步指令、途中 pearl 跨速度档、支付能力、墙、当前尾巴、碰撞前身体掉珠、两个 Queen 身份和最终 Queen 优先评分。
- 同一引擎再执行 WASM 版本：134 场景，其中 21 个完整策略场景使用官方 judge clang 和计量沙盒。最高单回合 12,653,701 CPU points（上限 100,000,000），没有沙盒错误。运动模型对照探针为原生 C++，引擎本身是官方 WASM；不能把 134 个场景都称为 134 个沙盒机器人策略测试。
- 新旧 Queen 都死亡后的比分状态、全地图胜率、完整 500 回合竞技对照尚未测量，不据此宣称胜率提升。

旧 verification/*_engine.py 多处自定义地图不按新引擎要求交替团队，还有把 ID 0/1 当成可献祭/撞头的普通龙的断言。旧 sprint_engine.py、feed_engine.py、portal_hold_engine.py、short_escape_engine.py 等结果不能直接当作新规则验证；当前有效的新规则入口是 new_rules_engine.py。旧记录保留用于历史追踪。

复现（使用新版工具包或指定其临时路径）：

```sh
python3 verification/run_checks.py
python3 verification/protocol_regressions.py --sanitize
c++ -std=c++20 -O2 -DFEAT_CHAMPION=0 -DFEAT_V3_SHORT_AVOID=0 -x c++ -I. verification/queen_rules.cpp.txt -o /tmp/v5-queen-ablation
/tmp/v5-queen-ablation
PYTHONPATH=/private/tmp/unswbc-queen-runtime python3 verification/new_rules_engine.py
PYTHONPATH=/private/tmp/unswbc-queen-runtime python3 verification/new_rules_engine.py --wasm
```

引擎输出 rounds=499 对应第 0–499 回合，共 500 回合。Queen 死亡本身不会提前终止比赛；验证专门保留了该队其他龙。

当前通过结果与沙盒点数见 verification/new_rules_results.json。旧 /Users/fanlingjun/Desktop/UNSW_battlecode/V5.zip 不会自动跟随源码更新，应从当前源码重新打包后再上传。
