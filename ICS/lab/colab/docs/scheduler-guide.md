# 调度器指南

## 你要实现什么

调度器要回答五个核心问题：

- 新就绪 task 放到哪个 worker
- 当前 worker 从本地队列里先跑谁
- tick 到来时是否抢占当前 task
- 某个 task 刚被唤醒时是否立刻抢占
- 空闲 worker 是否去别处偷任务

这些决策都发生在 `student/scheduler.h` 和 `student/scheduler.cc` 中。

## 参考实现

`benchmark/baseline_rr.cc` 里的 `BaselineRoundRobinScheduler` 是一个完整、正确、可以直接运行的参考实现。它的目标是提供一个清晰的最小基线：

- 放置逻辑简单
- 选取逻辑接近 Round-Robin
- 正确性优先
- 性能空间留给你自己的设计

先把这个实现读顺，再开始改自己的策略，推进速度会快很多。

## 五个主要 hook

### `select_worker`

决定一个新就绪 task 进入哪个 worker 的本地队列。

这一步常见的考虑因素有：

- 各 worker 当前队列长度
- 当前是否存在空闲 worker
- task 原来的 `preferred_worker`
- 跨 worker 迁移开销
- NUMA / node 拓扑

### `pick_next`

决定当前 worker 接下来运行哪个 task。

这一步通常最直接地体现你的赛道取向：

- throughput 更看重减少空转和提高整体完成速度
- latency 更看重尽快服务交互型或短任务
- fairness 更看重不同权重 task 的份额分配

### `on_tick`

时钟中断到来时，决定当前 task 是否让出 CPU。

这一步决定时间片风格、响应性和 cache locality 的平衡。抢占更积极，响应通常更好；连续运行更长，吞吐和 locality 往往更好。

### `should_preempt`

某个 task 刚被唤醒时，决定它是否立刻打断当前 task。

这一步对 latency 赛道很关键。很多交互负载的好成绩来自更快地处理 wakeup path。

### `steal`

当前 worker 空闲时，决定是否从别的 worker 那里偷任务。

这一步对 throughput 很关键。steal 的收益来自减少空闲，代价来自迁移成本和局部性损失。

## 三个重要视图

### `TaskView`

`TaskView` 是 task 的只读快照。最常用的信息包括：

- `task_id`
- `weight`
- `preferred_worker`
- `total_runtime_us`
- `current_slice_runtime_us`
- 阻塞次数和阻塞时间

这些字段可以帮助你区分长任务、短任务、交互任务和不同权重的 task。

### `QueueView`

`QueueView` 是某个 worker 本地 ready queue 的只读视图。

常见用法包括：

- 扫描候选 task
- 按自己的规则选一个最值得运行的 task
- 在 stealing 时分析受害者队列

### `SystemView`

`SystemView` 提供全局状态。

最常用的信息包括：

- 各 worker 的队列长度
- 各 worker 当前是否空闲
- 当前正在运行的 task
- `migration_cost(from, to)`
- worker 所属拓扑节点

很多 placement 和 steal 的决策都要结合 `SystemView` 做。

## 从哪里开始写

一个实用的起点是：

1. 先做一个稳定的本地队列策略
2. 再加一个简单的放置逻辑
3. 然后加 stealing
4. 最后细调 tick 和 wakeup preemption

这样更容易定位每一步带来的收益和回归。

## 常见失误

- 把同一个 task 返回给两个 worker
- `pick_next` 返回一个当前队列里不存在的 task
- steal 目标已经失效
- 抢占策略太激进，导致大量切换成本
- 只盯一个公开场景调参，迁移到其他场景后表现大幅波动

## 调试建议

优先用最小公开 workload 复现问题，再逐步扩大规模。

常用命令：

```bash
python tools/bench.py list
python tools/bench.py debug --scenario public/throughput/uniform_batch
python tools/bench.py debug --scenario public/latency/rpc_fanout_join
python tools/bench.py release --track throughput
```

当你看到一个 workload 分数变化时，先问自己两个问题：

- 这个 workload 在奖励哪种调度行为
- 我的策略在哪一步为这种行为加了分或减了分
