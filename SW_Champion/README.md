# SW_Champion — C++20 Battlecode bot

可直接交给官方 `unswbc` 的独立 bot 项目。`SW_Initial` 保留作原始对照；项目不读取地图文件、不依赖 Python、不改动现有 `benchmark.py`。地图信息只来自本龙当前视野与历史记忆。

在工作区根目录可先运行 `make test` 做规则检查，`make package` 重新生成提交包。对局命令：

```sh
unswbc run maps_BeforeSprinter/arena.map SW_Champion SW_Initial -o artifacts/champion.replay
```

提交时把 `SW_Champion` 作为 bot 目录；也可使用工作区 `artifacts/SW_Champion.zip`，zip 根目录直接包含 `bot.toml`。本次开发未提交线上排行榜。

## ABCD 接口与负责文件

| 角色 | 文件 | 边界 |
|---|---|---|
| A 策略、分裂、整合 | `strategy.hpp` | `Strategy::decide(ct)` 产出唯一 Action；`selectTarget` 选目标；`shouldSplit` 管理备用龙 |
| B 寻路、冲刺、战术 | `navigation.hpp` | `moveToward(Target)`、`moveSafely(Target)`、`escape()`、`intercept(Enemy)` 返回 Action，不直接输出 |
| C 地图、珍珠、声纳 | `world.hpp`、`communication.hpp` | `update(ct, game)` 更新观测；`remember(action)` 跟踪自身身体；查询接口见下 |
| D 测试与分析 | 项目外的 `tests/`、`artifacts/` | 官方 CLI 接口保持不变，已有 benchmark 可直接传入本目录 |

公共数据类型与调参入口在 `types.hpp`。主循环 `main.cpp` 只负责 `update → decide → execute → remember → end_turn`。生产版本不输出日志；需要回放诊断时定义 `CHAMPION_DEBUG`。

C 提供：

```cpp
Target WorldModel::closestPearl() const;
Target WorldModel::bestSpawnSoon() const;
bool WorldModel::isKnownSafe(Position) const;
std::optional<Position> WorldModel::portalDestination(int id, Position from, Direction) const;
std::vector<Position> WorldModel::portalDestination(int id) const;
int WorldModel::predictedSymmetry() const;
```

`portalDestination(id)` 返回已发现的门边锚点；唯一出口必须同时提供入口位置和方向，因为门户双向、双面。`predictedSymmetry()` 返回 0（未唯一确定）、1（x 反射）、2（y 反射）、4（180 度旋转）。对称推断目前只用于查询，不把推断当成已观测的安全通道。

`Target` 包含位置、目标类型和价值；`Enemy` 包含 ID、头坐标及可见长度下界；`Action` 包含 Move/Split、方向序列、childSize、评价分数和 safe 标记。safe 表示规划模型中动作可执行；有战争迷雾时不能保证真实世界绝对安全，拦截敌头是显式允许的交换。

## 策略

- 珍珠与出生计时记忆；过期珍珠信息降权，减少反复追逐已消失的资源。
- 环形地图与传送门的有向图寻路；未发现另一端的门仅在无已知生路时尝试。
- 跨回合维护自身头到尾的身体序列，模拟生长和冲刺额外掉尾。
- 最多 3 步即时冲刺候选，之后用有固定宽度与深度的 beam search 规划；首步碰撞检查包含尾巴。
- 评估可达空间、尾部释放时机、敌头下一轮的短距离冲刺威胁。
- 优先保存长度；常规分裂只在资源、出口和时间条件合适时增加少量备用龙。完全受困时允许紧急分裂，可能超过常规人口目标。
- 小龙仅在已有其他队友且敌龙可见长度明显更大时尝试兑子；最后一条龙不主动兑子。
- 声纳提供 32 位编解码接口，默认不开启发送、不信任收件内容。没有发送者身份的声纳不能作为安全路径证据。

计算有固定上限，不通过时钟控制搜索，因此行为可复现。地图最大 64×64，搜索最多 36 个 beam 节点、10 层、即时冲刺 3 步；参数可在 `Config` 调整。

## 验证

```sh
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic SW_Champion/main.cpp -o artifacts/champion
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined tests/test_bot.cpp -o artifacts/test_bot
artifacts/test_bot
clang++ -std=c++20 -O2 tests/greedy.cpp -o artifacts/greedy
python3 tests/verify_matches.py artifacts/champion artifacts/greedy
```

`tests/verify_matches.py` 使用安装的官方 0.3.6 引擎，交换 A/B 位置并保存独立回放、结果 JSON、异常回合输入；不调用或改动 `benchmark.py`。`tests/greedy.cpp` 是独立的 BFS 采珠与空间评估对照，不是排行榜强队代理。结果与局限见 `artifacts/VALIDATION.md`。

本地 native 耗时包含进程通信，不能换算成官方 WebAssembly CPU points；还需线上受限环境验证。战争迷雾中的远端出口占用、长距离敌方冲刺、未来敌方动作只能近似估计。没有对真实排行榜强队的评测，不能据本地胜率保证冠军。

规则核对来源：
- https://game.battlecode.au/docs/movement
- https://game.battlecode.au/docs/execution-order
- https://game.battlecode.au/docs/kelp-and-portals
- https://game.battlecode.au/docs/splitting
- https://chatgpt.com/share/6ab13325-4470-83ed-9d06-1ccc574378ba
