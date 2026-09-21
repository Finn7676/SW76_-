# UNSW Battlecode Quickstart 下载内容

来源：<https://game.battlecode.au/docs/quickstart>  
官方工具版本：`unswbc 0.3.6`  
下载日期：2026-09-21

Quickstart 没有提供独立的题干 PDF 或题目压缩包。官方给出的“题目下载/开始方式”是安装 `unswbc` 后生成起始 bot，并把工具内置地图复制到 `maps/` 目录。本目录保存了该流程得到的 Python 起始文件和地图。

## 目录内容

- `python/main.py`：Python 起始 bot；
- `python/helper.py`：读写比赛引擎协议的辅助库；
- `maps/*.map`：官方工具随附的本地测试地图，包括 `arena.map`；
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

## 注意

这些文件是起始工程和本地测试地图，不是单独的自然语言“题干”。完整比赛规则仍以网站文档为准：<https://game.battlecode.au/docs>。

