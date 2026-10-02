# Queen 探索禁用传送门（2026-10-02）

Queen（永久 ID 0/1）的探索改为普通行走。禁用开局探门、Scout 门任务和探索候选评分中的传送门；探索 BFS 不使用门户捷径，BugNav 包括绕墙恢复路径都避开门。最终动作检查拒绝探索 MOVE 过门，也逐步检查 sprint 中途过门。

已有 Queen Scout 任务会清除。探索时优先正常行走，必要时分裂脱困；若没有任何合法普通一步且不能分裂，则转为 EMERGENCY 生存模式，再考虑唯一门户逃路，避免把无动作当作等待而让 Queen 死亡。已经困在门户房间内的退出也属于 EMERGENCY。

规则限定 EXPLORE。明确珍珠采集属于 COLLECT，仍可使用已知安全门户；普通龙继续原来的 Scout 探门。Queen 原来的死胡同、身体碰撞和避敌检查继续执行。

24 组 C++ 回归、Queen 探索专项 ASan/UBSan、7 个历史协议输入 ASan/UBSan 通过。官方 unswbc 1.2.3 执行 11 个 WASM 快照，覆盖双方 Queen、开局和后期、已知与未知门出口、明确采集，以及普通龙 Scout。最高 5,624,392 CPU points，小于 100,000,000。专项 C++ 还检查 sprint 第二步过门、仅门可逃、纯步行 BFS 无法跨门。

引擎快照仅替换策略 ROUND 标签，没有预置地图或 bot 历史；引擎物理和 ID 执行顺序保持原样。没有做完整对局胜率比较。源码指纹（文件名+内容）：`ca47a96bf4e188d47c96556e13ff7dfa698c87fe6c1835154d69298526e3c893`；详细结果：`verification/queen_explore_validation.json`。之前其他验证文件是历史记录。
