# coLab 报告

姓名：王松宸
学号：2024201594

## Part A: 思路简述

<!-- 200字以内简述 Fairness 赛道的实现思路,重点说明:
1. 核心方法的流程，依次做了什么
2. 关键数据结构的设计，如何组织和管理作业控制
-->

目标是让各 group 按权重比例获得 CPU。调度器维护每组的 `service_us/weight` 作为基础公平键，并用指数衰减窗口统计 `actual/target` 形成 window debt，逼近评分器 `fairness_share_skew` 的窗口 skew。每次 `pick_next/steal` 先扫描系统得到活跃组与各组 running/runnable 并行度，再对候选按 `fair_key + debt + oversub 罚分` 取最小；在 4:1 双组无阻塞时加软 admission gate 防止轻组出现“窗口内零服务”。对睡眠/阻塞后再激活的组做 service clamp + burst credit；`on_tick/should_preempt` 用最小片、slack 与权重量化 quantum 控制切换开销，并在唤醒路径尽量保留局部性。

## Part B: 具体实现分析

### 模块1——公平性状态维护
<!-- 300字以内,描述代码中你认为的模块及其对应内容
-->

通过 `task_accounted_runtime_us_` 做增量记账，把每个 task 新增 runtime 累计到 `group_service_us_`；`group_weight_` 保存权重，`group_boost_us_` 保存再激活补偿（用于抵消历史 service 带来的劣势）。同时实现窗口债务：`account_window_runtime_locked` 以 half-life 指数衰减维护 `group_window_actual_us_`/`group_window_target_us_` 与 `window_target_total_us_`，其中 target 会按活跃组权重份额分摊；`window_debt_key_locked` 计算 (actual-target)/weight 并裁剪到 cap。`refresh_active_groups_locked` 负责识别 reactivation 并做 service clamp + burst credit，必要时加 window reactivation floor，避免睡眠组被长期压制。

### 模块2——评分器与选人逻辑
<!-- 300字以内,描述代码中你认为的模块及其对应内容
-->

`pick_next`/`steal`先遍历 `SystemView` 汇总 `active_groups`、`running_by_group`、`runnable_by_group`，并计算活跃组的最小/最大权重与是否出现阻塞，据此选择 window debt 的启用范围：4:1 双组无阻塞时走 narrow gate；满足比例/最小窗口时长等条件时走 broad debt（并对阻塞场景更保守）。随后对候选任务计算 group_score：`fair_key`（`service/weight`）叠加 debt，再加上 oversubscription 罚分；期望并行度按 `workers * weight/total_weight`，超配部分按 switch_cost 缩放，且对轻组施加更强的惩罚以避免“瞬时占满核”。当活跃组为 4:1 时还启用软 admission gate，按份额限制每组可占用的 running 槽位，防止轻组出现 window 内实际为 0 的极端 skew。`steal` 在 idle 时全局扫描 victim 队列，优先同 NUMA 节点并过滤过高迁移成本，且刻意忽略 oversub 去偷“最落后/最欠债”的组来加速追平。

### 模块3——放置与抢占控制
<!-- 300字以内,描述代码中你认为的模块及其对应内容
-->

`select_worker` 在 wakeup 路径优先回到 source/previous worker，并用 load slack 判断是否迁移，从而优先保留 cache 局部性；否则选择全局最轻负载，并在并列时偏向同 NUMA 节点。时间片由 `quantum_for_weight` 计算：以 `max(min_q, switch_cost*scale)` 为基准，按权重增加 bonus，并用 max_q 截断，避免过短 quantum 导致切换开销放大。`on_tick` 除了量化到期外，还比较当前组与队列中最欠服务组的 `service/weight`，在超过 tick_slack 时触发 resched。`should_preempt` 同时考虑 admission、wakeup slack（再激活时收紧）与“catch-up preemption”（将 window debt 以 0.05 倍阻尼引入），在提升公平的同时尽量避免 daemon 场景因过度抢占而性能崩盘。

## Part C: 关键难点解决
<!-- 选择2-3个最有技术含量的难点:
1. 具体难点描述
2. 你的解决方案
3. 方案的效果
-->

1) **难点**：评分器使用窗口内“目标份额 vs 实际份额”的 skew（并取 90th percentile），单纯用累计 `service/weight` 很难捕捉阶段切换与短窗口的偏差。
**解决**：维护指数衰减窗口 `actual/target`，把每次新增 runtime 按活跃组权重份额分摊到 target，计算 window debt=(actual-target)/weight 并裁剪；在 `pick_next/steal` 里作为附加 key 使用，并用 narrow/broad gate 控制适用范围。效果：对 burst arrival、sleepy reactivation 等场景的短时偏差更敏感，能减少“窗口内零服务”导致的 skew 爆炸。

2) **难点**：多核下某个组很容易瞬时过度并行（oversub），即使长期份额接近，短窗口内也会严重偏离目标。
**解决**：在 group_score 中引入 expected_parallelism（按权重份额分配核数）并对超配施加罚分；罚分随 switch_cost 缩放且对轻组更强，从机制上抑制“重组占满所有核”。同时在 4:1 双组无阻塞时启用软 admission gate，按 runnable/份额估计可用槽位并限制进一步进入 running。效果：在 `daemon_vs_requests`、`elephant_vs_mice` 等极易失衡的 workload 中更稳定地维持份额，避免轻组被饿出窗口。

3) **难点**：在抢占路径引入过强的公平信号会导致频繁切换、吞吐下降，且某些 daemon 型场景对抢占非常敏感。
**解决**：设置最小抢占片（8us）、tick/wakeup slack 与权重量化 quantum，先控制切换频率；window debt 不进入 `on_tick` 的主要比较，而只在 `should_preempt` 里以 0.05 倍阻尼做“catch-up”辅助（欠债的 waking 更容易抢占、过度服务的 current 更容易让出）。效果：在不显著破坏吞吐的前提下改善唤醒后的追平速度，降低因为过度抢占导致的回归风险。

## Part D: 实验反馈
<!-- 你的反馈对我们至关重要
可以从实验设计，实验文档，框架代码三个方面进行反馈
-->

实验设计：整体难度和工作量比较匹配，尤其是 Fairness 赛道需要同时考虑多核并行度、切换成本与份额追踪，能迫使我把“公平”从直觉变成可量化的机制。

实验文档：`docs/scheduler-guide.md` 和 workload 相关文档对入门很友好；建议在文档里更集中地解释 Fairness 的计分细节（例如 window skew 的定义、90th percentile、以及 makespan 超过 baseline 2 倍时封顶为 1.0 的规则），并给出一两个手算/示例 trace，能显著降低“盲调参”的成本。

个人思考：AI 打榜哪家强，你 G 老师是永远的 King。
