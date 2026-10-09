# UNSW Battlecode Quickstart 下载内容

来源：<https://game.battlecode.au/docs/quickstart>  
官方工具版本：`unswbc 1.2.7`
更新日期：2026-10-03

Quickstart 没有提供独立的题干 PDF 或题目压缩包。官方给出的“题目下载/开始方式”是安装 `unswbc` 后生成起始 bot，并把工具内置地图复制到 `maps/` 目录。本目录保存了该流程得到的 Python 起始文件和地图。

## 目录内容

- `python/main.py`：Python 起始 bot；
- `python/helper.py`：读写比赛引擎协议的辅助库；
- `maps/*.map`：官方工具 1.2.7 随附的本地测试地图；
- `bot.toml`：由 `unswbc init python mybot` 生成，内容如下：

```toml
# created by `unswbc init`
[project]
language = "py"
include = [ "*.py" ]
```

## 官方获取命令

```bash
pip install unswbc
unswbc init python mybot
unswbc maps
unswbc run maps/arena.map mybot mybot
```

其中 `unswbc init` 会创建 `main.py`、`helper.py`、`bot.toml`，并在 bot 项目旁边创建共享的 `maps/` 目录；`unswbc maps` 会补充新版本工具附带的地图。

## 地图版本

当前目录已与 `unswbc 1.2.7` 工具包的 **22 张地图逐字节同步**：`Colosseum`、`arena`、`australia`、`autarky`、`big_empty`、`default`、`default_small`、`devil`、`dilemma`、`islands`、`maze`、`portals`、`queen_of_spades`、`schooltime`、`slithery_fight`、`stripes`、`stronghold`、`tower_defense`、`trauma`、`trophy`、`unsw`、`weakhold`。

相对原目录新增 7 张：Australia、Islands、Maze、Stripes、Tower Defense、Around UNSW、weakhold。10 月 2 日公告确认地图修订及 Sprint 地图已上线。

### 排位地图池（2026-10-03 核验）

官网 [排位对局页](https://game.battlecode.au/games?kind=ranked-autoscrim,ranked-scrim) 的两页近期对局、每页 100 条记录（分页期间新增对局，合计 199 个不同对局 ID），均恰好覆盖以下 **17 张地图**：Australia、Autarky、Default、Devil、Portals、Prisoners Dilemma、Queen Of Spades、Schooltime、Slithery Fight、Trauma、Trophy、Islands、Maze、Stripes、Tower Defense、Around UNSW、weakhold。两页的所有记录均标记 `ranked=true`。

相对工具包的 22 图，排除 **Colosseum、Arena、Big Empty、Default Small、Stronghold**。这是官网近期实际排位用图的核验；`/api/v1/maps` 仍需要登录，未读取其完整配置。地图池以后可能变动，benchmark 保存核验日期与来源。

官方 1.2.7 的 `unswbc update` 会同时更新 helper 和地图；请将工具版本与地图目录绑定记录，避免用旧地图解释新引擎结果。

`unswbc maps` 仅添加缺失地图，不覆盖旧内容。仅同步本目录的全部工具包地图可执行：

```bash
.venv-battlecode/bin/python -c 'from pathlib import Path; from unswbc.maps import replace_changed; replace_changed(Path("battlecode_quickstart/maps"))'
```

## 本地 benchmark

项目独立环境 `.venv-battlecode` 已安装 `unswbc 1.2.7`。`python3 benchmark.py` 自动使用该环境，使准备、编译与对局引擎一致；没有该环境时，要求当前解释器安装 `unswbc >= 1.2.7`。

```bash
python3 benchmark.py xxqz SW_Champion --maps all --games 2 --seed 42
python3 benchmark.py xxqz SW_Champion --maps maze tower_defense --games 2 --sandbox
```

`all` 或 `ranked` 使用已核验的 **17 张排位地图**，两个 bot 参数且省略 `--maps` 时也默认采用该集合；`bundled` 使用全部 22 张工具随附地图。每次检查所选地图与工具包内容一致，拒绝缺失或过时文件。可以按名称显式选择地图；历史地图目录仍保留。每个 A/B 对局对使用相同种子，下一对递增；省略 `--seed` 时每对随机生成种子。`run.json` 记录地图选择、排位基线来源和核验日期、工具版本、规则、sandbox 模式和地图 SHA-256，`results.csv` 记录各局种子。普通模式不验证裁判 CPU points；提交前应另用 `--sandbox` 检查预算。

本机默认 `c++` 不支持官方工具所用的 `-std=c++20`；普通模式的 C++ 测试可显式使用已验证的 `CXX=g++-10 python3 benchmark.py ...`。`--sandbox` 使用工具自带的裁判 clang，无需该设置。本次验证：全部 22 图、每图两局共 44 局 Python 自测通过；Python sandbox、C++ 普通模式和 C++ sandbox 各两局验证通过。这些是运行链路验证，不代表任何参赛策略的强弱结论。

## 协议版本

当前 C++ 与 Python helper 使用协议 v3。每回合可向四个方向各发送一个 64 位声呐，并提供海带、友军、友军头部、敌军和敌军头部的回声计数。旧式 `send_sonar(message)` 调用仍保留兼容性，但只沿当前朝向发送 32 位值。

## 注意

这些文件是起始工程和本地测试地图，不是单独的自然语言“题干”。完整比赛规则仍以网站文档为准：<https://game.battlecode.au/docs>。
