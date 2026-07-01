# Workload 指南

## `.sched` 文件在描述什么

每个 `.sched` 文件都是一个 benchmark 场景。它描述：

- 有多少 worker
- CPU 和设备的抽象速度
- context switch 和迁移开销
- task 在什么时间到来
- task 会执行哪些 `compute / call / sleep / sync` 操作

公开 workload 位于：

- `workloads/public/throughput/`
- `workloads/public/latency/`
- `workloads/public/fairness/`

排行榜使用 `role leaderboard` 的场景；`role gate` 的场景用于 release 模式下的 correctness gate。完整的 public/hidden 排行榜场景清单维护在内部文档 `docs/leaderboard-workloads.md`。

## 先看一个最小骨架

```text
track throughput
score throughput_sustained_rate
workers 4
cpu_rate 16
switch_cost 2us
migration_cost local=3us remote=9us

device net0 fifo rate=8

phase main at 0us:
  group clients * 8 arrival stagger 200us:
    repeat 5:
      compute 40
      call net0 20
      sleep 1ms
```

读这个文件时，可以按下面的顺序理解：

1. 这条赛道在优化什么
2. 有几个 worker，机器拓扑是什么
3. task 什么时候到来，多少个并发
4. 每个 task 的执行路径是什么
5. 哪些开销会影响调度选择

## 常见顶层字段

### `track`

表示场景属于哪条赛道：

- `throughput`
- `latency`
- `fairness`

### `score`

表示这个场景用什么指标计分。学生调策略时主要关心它与赛道目标是否一致。

### `workers`

表示这个场景里有多少个 worker，也就是多少个可调度的执行核心。

### `cpu_rate`

表示 `compute` 这类 CPU work 的抽象执行速度。

### `switch_cost`

表示同一 worker 上切换 task 的额外代价。抢占越频繁，这个代价越容易变得重要。

### `migration_cost`

表示 task 从一个 worker 迁移到另一个 worker 的代价。

如果场景定义了 `node`，`local` 和 `remote` 会区分同节点与跨节点迁移成本。

## 常见行为块

### `phase`

表示一个时间阶段，从某个时间点开始释放一组 group。

### `group`

表示一批行为相同的 task。

`group clients * 8` 表示创建 8 个行为一致的 task。

### `arrival`

表示 group 内 task 的到达方式。常见形式包括：

- `delay`
- `stagger`
- `interval`
- `burst`

arrival 形状会直接影响 latency 和 burst handling。

### `compute`

表示一段 CPU work。

### `call`

表示一次设备请求，例如磁盘或网络请求。它通常会让 task 阻塞，之后由设备完成事件唤醒。

### `sleep`

表示 task 主动休眠一段时间，之后再被唤醒。

## 调度器最关心哪些 workload 信号

### throughput 场景

优先观察：

- 队列是否容易失衡
- 是否需要 stealing
- 是否存在明显 locality 机会
- switch 和 migration 成本是否显著

### latency 场景

优先观察：

- 交互 task 的 arrival 形状
- wakeup 后是否需要快速抢占
- 背景 batch task 会不会淹没前台请求

### fairness 场景

优先观察：

- group 的权重
- 长任务与短任务的混合方式
- 干扰负载是否会挤压低份额 task

## 建议怎么读公开 workload

一个有效的方法是：

1. 先读文件名，猜这个场景想测什么
2. 再看 `track`、`score`、`workers`
3. 然后画出 phase 和 group 的时间线
4. 最后结合 `switch_cost`、`migration_cost`、设备调用和同步关系，判断这个场景会偏爱什么策略

## 常用命令

```bash
python tools/bench.py list
./build/benchmark/runner --list-workloads --suite public --role leaderboard
python tools/bench.py debug --scenario public/throughput/cache_affinity
python tools/bench.py debug --scenario public/latency/rpc_fanout_join
python tools/bench.py debug --scenario public/fairness/three_tenants
```

公开 workload 的价值主要有两点：

- 帮你理解每条赛道在奖励什么
- 帮你在提交前发现策略在别的场景上的回归
