# V3 主动 NONE 的代码语义

日期：2026-09-28。仅核实冻结源码语义与已执行测试；未修改生产源码，未重跑比赛。本记录未逐个核验报告涉及的 18 个死亡 code 4 事件。

## 完整执行链条

- `V3_portal/planner.hpp:501–528`：`scoreAndFallback()` 在普通候选不足时尝试紧急分裂、敌方头碰撞和后备方向；后备方向仍排除队友头及不值得进入的小口袋。如果最后没有候选，明确 `return {}`，得到 `ActionType::NONE`。该行注释是 `default self-sacrifice spares teammate heads`。这是一条有意保留的策略分支，不需要进程异常才能发生。
- `V3_portal/planner.hpp:29–37`：`avoidTeammateHead()` 检查已选移动或冲刺路径，一旦命中已知队友头便直接返回 NONE。该函数不重新搜索其他移动或分裂，因此不能把所有此类 NONE 描述为“已经穷尽所有替代动作”。未知门户出口会中止检查，也不能据此获得完整安全保证。
- `V3_portal/main.cpp:48–56`：主循环先调用 `choose()`，再调用上述末端过滤器；当动作为空时清除活动 Scout 声明，随后仍调用执行器。
- `V3_portal/action.hpp:32–46`：`Executor::execute()` 的 NONE 分支仅 `break`，不输出 `MOVE` 或 `SPLIT`。
- `V3_portal/main.cpp:61–64`：主循环仍可发送声呐，并执行 `end_turn()`。因此“本回合没有移动/分裂命令”与“进程已经崩溃”不是同一个事实。`V3_NOTES.md:18` 也明确描述过不输出动作、仅牺牲当前龙的设计。

## 已执行的测试证据

原有 `verification/v3_tests.cpp.txt` 明确把以下行为作为预期断言：

- 第 140–149 行：单珍珠小口袋被拒绝，其他出口封闭后返回 NONE。
- 第 171–183 行：对进入队友头的移动和冲刺，末端过滤器返回 NONE；队友头占据唯一剩余出口时，规划器也返回 NONE。

本次已实际执行，无需追加测试：

```sh
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Werror -I tyhh_test/V3_portal -x c++ tyhh_test/V3_portal/verification/v3_tests.cpp.txt -o /tmp/v3-none-source-tests
ASAN_OPTIONS=detect_leaks=0 /tmp/v3-none-source-tests
```

退出码为 0，输出 `V3 focused checks passed`，未出现 ASan/UBSan 报错；LSan 因沙箱环境关闭。这验证了测试场景中的预期行为，不是对所有真实回合的行为证明。当前 `main.cpp`、`planner.hpp`、`action.hpp` 三个文件的 SHA256 均与 `relay_final/sources/1/V3_portal/` 内的冻结快照一致。

## 对 code 4 的解释边界

源码证明 V3 存在主动不给出 MOVE/SPLIT 的执行路径，可解释一类无动作死亡。单个死亡代码本身不足以区分主动 NONE、其他非法动作或运行异常。报告涉及的 18 个 code 4 事件，不能在缺少当轮输出、错误文本或现场状态逐条核验的情况下，全部宣称为已确认的策略自杀；同样不能把它们全部等同于进程崩溃。

比赛进程失败、单条龙的回合错误和主动无动作死亡应分别表述。归因应以具体事件证据为准；本记录不调整任何胜负统计，也不将策略合理性当作竞争力证明。
