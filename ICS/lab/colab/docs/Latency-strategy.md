# Latency Scheduler Strategy

## 当前基线

**v3-public (hybrid reactive MLFQ) — public 1.938x, hidden 1.332x**。

v1 提交号：`019e14e3-8444-7c33-a130-2802dd928641`

## 计分机制

两种 scorer：
- `latency_wakeup_p99`: quality = 1 / p99_wakeup_latency_us
- `latency_flow_p99`: quality = 1 / p99_flow_time_us
- 吞吐保底：student_throughput / baseline < 0.8 时 score 被限制

## 架构：双模式调度器

全局 `seen_background_weight_` atomic —— 任何 `weight > 1` 的任务出现即永久进入 mixed mode。

### 任务分类 (3-Tier)

| Class | Criteria | Quantum (pure) | Quantum (mixed) |
|-------|----------|---------------:|----------------:|
| 0 Continuation | `has_blocked_before()` — `voluntary_block_count > 0 \|\| last_stop_reason == Blocked` | 20us | 24us |
| 1 Fresh FG | `weight <= 1`, 无 block 历史 | 20us | 24us |
| 2 Background | `weight > 1`, 无 block 历史 | — | 1us (fg waiting), 64us (bg only) |

`has_blocked_before()` 是粘性标记，一次阻塞永久成为 class 0。

### Pure Interactive Mode（未观测到后台）

- `pick_next`: FIFO
- `select_worker`: spawn round-robin；wakeup 同 worker 回 source
- `on_tick`: 仅本地队列非空时 20us 量子
- `steal`: 禁用
- `should_preempt`: 仅 waking 曾阻塞 且 (waking 全新 或 current 未阻塞)

选型依据（pure-mode A/B）：

| 变体 | periodic | smoke | storm |
|------|---------:|------:|------:|
| aggressive everywhere | 0.420x | 1.000x | 23.333x |
| FIFO/source + steal | 0.788x | 0.333x | 2.917x |
| **FIFO/source + no steal (当前)** | **0.925x** | **0.333x** | **2.917x** |
| +空队列也强制 20us quantum | 0.793x | 1.000x | 2.917x |

牺牲 smoke/storm 分数保护 hidden h04/h06（更像 pure periodic）。

### Mixed Latency Mode（观测到后台后）

- `pick_next`: 按 `(class, total_runtime, weight, queue_index)` 优先级
- `select_worker`: foreground spawn/wakeup 用 `foreground_waiting_load()` 分布；background spawn round-robin
- `on_tick`: 四级检查 — meaningfully_better@1us, fg-vs-fg@24us, bg-before-fg@1us, bg-only@64us
- `steal`: 从队列最大(>1)的 worker 偷最优任务；保守避免偷 bg（queue≤1 且 migration>2×switch）
- `should_preempt`: `meaningfully_better(waking, current)` 且 slice≥1us

`foreground_waiting_load`: queued continuation=+2, fresh=+3, running fg=+2/3, preferred discount=-1。

## 场景分数 & 当前瓶颈

### Public leaderboard（v3-public, 1.938x）

| Scenario | Score | 瓶颈 |
|----------|------:|------|
| wakeup_storm | 23.333x | gate only, 已满分 |
| priority_inversion | 3.618x | 接近上限 |
| bursty_request_queue | 2.063x | 良好 |
| rpc_fanout_join | 1.857x | 可小优化 |
| interactive_vs_batch | 1.653x | 中等 |
| service_cascade | 1.598x | **最大空间，p99 ~428us, 目标 ~340us** |
| load_ramp | 1.490x | 中等 |
| interactive_periodic | 0.918x | pure-mode proxy for hidden h04/h06 |
| wakeup_smoke | 0.333x | gate only, 根因未解 |

### Hidden（v1, 1.332x）

| Scenario | Score | 判断 |
|----------|------:|------|
| h01 | 1.544x | mixed, v1 前台抢占有效 |
| h02 | 1.427x | mixed, v1 前台抢占有效 |
| h03 | 1.703x | mixed, v1 前台抢占有效 |
| h04 | 1.020x | 接近 baseline，疑似 pure/wakeup |
| h05 | 1.246x | mixed-pure 过渡 |
| h06 | 1.170x | 接近 baseline，疑似周期唤醒 |

旧 throughput 风格调度器 hidden latency: 0.123x / 0.175x，不做前台抢占会被部分 hidden 场景打爆。

## 公开 workload 结构

所有 mixed 场景共同特征：前台 request `weight=1`，后台 batch `weight=4` 或 `weight=6`。前台 CPU burst 8~16us + sleep 40~100us，后台 `compute 24000~36000` 的长 CPU 任务。

核心矛盾：**前台短任务 flow tail vs 后台长任务吞吐保底**。

## 调优历史

| 策略 | Public | 关键变化 |
|------|-------:|----------|
| 旧 throughput/fairness | N/A | hidden latency 0.12x~0.18x |
| v1 三层优先级 + fg quantum 12 + bg quantum 5 | 1.740x | 初版 reactive MLFQ |
| fg quantum 64 + bg quantum 3~4 | 1.764~1.847x | public 逐步提升 |
| bg-only 64us, bg 遇 fg 立即让路 | 1.874x | 吞吐 guardrail 确立 |
| v1 去掉 deep-continuation bias | 1.889x | → **提交 hidden: 1.332x** |
| v2 hybrid pure/mixed mode | 1.889x | interactive_periodic 0.420→0.925，未提交 hidden |
| v3 fg-only placement load + source tie-break | **1.938x** | 当前稳定基线 |

### 已确认无效/负面的尝试

- `background quantum=2`: public → 1.751x
- `background-only quantum=32us`: 伤 bursty_request_queue
- `deep-continuation priority`: 新 request 排得更久，P99 变差
- `foreground quantum 4/12/20/32/64`: 对 public 无差异
- `foreground spawn round-robin`: 无稳定收益
- `fresh-before-continuation`: 明显负收益
- `foreground LIFO tie-break`: 无收益
- `forced wakeup-to-source`: 负收益

### 本轮探索（2026-05-11，target 2.1x）

所有尝试均未能突破 1.938x，代码已回退基线：

| 方向 | 最佳分数 | 失败原因 |
|------|---------|----------|
| per-worker blocked FG 跟踪 + placement 避让 | 1.847~1.925x | load_ramp/rpc_fanout_join/priority_inversion 回退 |
| migration cost 纳入 placement load | 1.847x | wakeup_storm 23.333→1.489，跨 worker placement 严重恶化 |
| continuation wakeup 优先回 previous worker | 1.840x | 负载不均 |
| adaptive fg quantum (队列≥3 时 12~18us) | 1.918x | 轻微负面 |
| steal 更激进 (queue_size=1 也偷 fg) | 1.925x | load_ramp 回退 |
| SJF continuation 抢占 (runtime 差>8us) | 1.930x | service_cascade 反而微降 |
| blocked FG 作为 placement tiebreaker | 1.925x | load_ramp/service_cascade 回退 |

**结论**：v3-public 处于局部最优，所有常规旋钮已穷尽。进一步突破需要框架提供任务剩余时间可见性、任务依赖图或更细粒度的 worker backlog 模型（阻塞任务准确计数），这些在当前 API 下不可得。

## 下一步建议

1. **当前 v3-public (1.938x) 直接提交 hidden** 作为第二轮
2. 如果 hidden 反馈指向 pure-mode 短板 (h04/h06)，回到 pure-mode wakeup placement 方向
3. 如果 hidden 反馈指向 mixed-mode flow tail，需要框架级改进（per-worker blocked task 准确计数等）
4. `service_cascade` p99 ~428us → ~340us 是理论上最大空间，但常规手段已耗尽

## 运行命令

```bash
cmake --build build --parallel $(nproc) --target runner
python tools/bench.py release --track latency
python tools/bench.py debug --scenario public/latency/<name>
python tools/bench.py debug --scenario public/latency/<name> --scheduler baseline
```
