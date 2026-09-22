# tyhh_rl

这是一个受决策时间限制约束的轻量强化学习接口原型。它没有把碰撞检测、传送门合法性或冲刺支付交给神经网络，而是复用 `SW_Champion` 的规则世界模型和安全导航器。

## 当前实现

- `LinearPolicy` 是一个固定大小的上下文价值模型，输入珍珠置信度、生成倒计时、探索前沿、距离、敌方危险、重复访问、传送门和拥挤程度。
- `Strategy::selectTarget()` 用模型给所有可达格子评分，再交给 `Navigator` 做合法路径规划。
- 模型没有动态内存、文件访问或外部服务调用，推理是固定数量的浮点乘加。
- 现有的分裂、碰撞、身体跟踪和紧急动作逻辑保持不变。

## 编译与运行

```bash
clang++ -std=c++20 -O2 -Wall -Wextra -Wpedantic tyhh_rl/main.cpp -o /tmp/tyhh_rl_bot
/tmp/tyhh_rl_bot
```

本地对局：

```bash
unswbc run maps_BeforeSprinter/arena.map tyhh_rl SW_Champion
```

## 下一步训练接口

当前权重是可解释的离线初始权重，不是声称已经训练完成的模型。后续可将自对弈数据中的目标收益、存活长度和最终胜负用于离线拟合或 PPO/MAPPO 训练，再把导出的 8 个权重替换到 `strategy.hpp`。

替换权重时应保留固定推理和规则安全层，并用最坏回合耗时而不是平均耗时验收。
