# Fairness Strategy

## Current State

- Public fairness: `4.162x` (stable)
- Hidden fairness: `2.16959` (new best; +0.0010 from previous 2.168638)
- Hidden-like local suite: geomean `1.767`, 300 cases across 14 families (still not correlated with hidden)
- **2026-05-14 end-of-day**: 10 quotas used, 9/10 returned exactly 2.16959. Only catch-up preemption moved hidden.

Hidden score distribution (last known detailed):

| Scenario | Score | Character |
|----------|------:|-----------|
| h01 | 1.000 | Hard floor — no mechanism has moved it |
| h02 | 4.527 | High, stable — protect |
| h03 | 1.185 | Core weakness — resistant to all tuning |
| h04 | 5.578 | High, stable — protect |
| h05 | 2.905 | Mid, fragile — must never regress |
| h06 | 1.184 | Core weakness — broad window debt briefly raised to 1.211 |

## What Works (Proven Mechanisms)

1. **Group-level fairness base**: ranking by `service/weight` with oversubscription penalty — foundational
2. **Soft 4:1 admission gate** (slack=1.0): protects h05 from zero-service
3. **Reactivation clamp**: lifted hidden from 2.080→2.150
4. **Multi-group reactivation credit** (credit=16, >=3 groups): low-risk probe
5. **Conservative preemption**: `wakeup=4`, `tick=6`, `min_slice=8`
6. **Quantum scaling**: `scale=7`, `bonus=2`, `min=24us`, `max=80us`
7. **Wakeup locality**: `locality_load_slack=1`
8. **Broad window debt (no-block only)** (2026-05-13): min_target=1000us, soft floor via `window_selection_debt_key_locked`, 0.5x scale. +0.0022 hidden.
9. **Global steal scan** (2026-05-13): scan all worker queues for best fairness task instead of just largest queue. +0.0015 hidden.
10. **Catch-up preemption** (2026-05-14): in `should_preempt`, add heavily-damped (0.05x) window debt signal — waking-underserved gets negative score boost, current-overserved gets positive boost. +0.0010 hidden.

## Dead Ends (Do NOT Reattempt)

### Admission / throttling
- Hard admission (slack < 0.5): h05→0.000. Never again.
- `admission_slack < 1.0`: h02/h04/h05 regress, h03/h06 don't move.
- `admission_slack >= 1.25`: no benefit; 2.0 weakens public.
- `>=3:1` admission generalization: crashes public daemon (9.682→2.482).
- Preempt-only admission: public drops to ~3.944.

### Threshold sweeps
- `wakeup/tick/min_slice` sweeps: hidden insensitive.
- `low_positive_ratio/min_target` under narrow 4:1: no hidden signal (round O).

### Window debt
- Broad window debt WITH blocking (even catch-up-only with -3.0 threshold): zero additional benefit. Don't waste quota.
- Broad window debt + reactivation floor (round L): h06↑ but h03/h05↓.
- 4:1 high-weight-only (round N): lost h06 signal.
- 4:1 low-weight soft-floor (round O): no h06 signal.
- Enabling window debt for daemon-like scenarios (5:1 with blocking): crashes public daemon 9.682→2.455. Must keep `!active_has_blocked` gate for ratios other than exactly 4:1.

### Reactivation
- Aggressive burst credit: local 2.759x regression.
- Weighted/reactivation/wakeup/idle tuning: maxed out.

### Preemption
- Catch-up preemption damping > 0.05x (e.g. 0.10x): daemon regression (9.682→3.442).
- Window debt in `on_tick` score comparison: daemon regression (9.682→3.060).
- Lower `min_slice_before_preempt` (8→4): daemon regression (9.682→3.290).

### pick_next / selection
- Starvation prevention bonus (unconditional or debt-gated -2.0/-6.0): daemon regression or zero benefit.
- Per-worker group stickiness (-2.0 bias for same-group): daemon regression (9.682→2.673).

### Quantum
- Adaptive quantum by fairness position (all 3 variants): public regression (3.150-3.943) and hidden regression (2.138).

### Placement / locality
- Increased wakeup locality (locality_load_slack 1→2): daemon regression (9.682→3.797).
- NUMA-aware tiebreaking in `select_worker`: zero effect.
- Migration cost weighting in `steal`: zero effect.

### Window debt tuning
- `min_target` 1000→500: zero effect on hidden.
- Broad scale 0.5→0.75: zero effect on hidden.

### Steal
- Steal ignores oversubscription (idle worker steals most underserved group regardless of instance count): zero effect on hidden.
- Two-tier steal (debt < -5.0 gated group priority): zero effect on hidden.

### pick_next
- Two-tier pick_next (debt < -5.0 gated group priority, +2.0 safety margin): zero effect on hidden.
- Window debt half-life 1000→2000: zero effect on hidden.

### on_tick
- Group time slice (suppress preemption when debt < -5.0 and !has_blocked): daemon regression (9.682→5.740). Same root cause as rule #23 — any debt signal in on_tick destabilizes daemon.

### Mechanism design rules
- Do NOT reject a mechanism because public drops from 4.164→3.572: round L/M h06 signal appeared when public dropped.
- Do NOT assume hidden moves when public stays flat.
- Do NOT add window debt to `should_preempt` or `on_tick` beyond 0.05x catch-up: daemon regression.
- Do NOT enable window debt for scenarios with blocking history: daemon regression proven 2026-05-13.
- Do NOT add biases to group_score (starvation, stickiness): daemon regression.
- Do NOT adjust quantum based on fairness position.

## Key Insight from Trace

Round M isolated the h06 signal source: broad window debt in `pick_next`/`steal` only (not preempt). h06 went 1.184→1.211. Signal disappeared when narrowed to 4:1 no-block only.

**2026-05-13 confirmation**: Expanding window debt coverage (broad + no-block) consistently improves hidden (+0.0022). The signal is real but small — broader coverage alone won't reach 2.30+.

## Hidden-Like Test Suite

Located at `docs/fairness_hidden_like_suite.py`. 300 cases across 14 families.

Usage:
```bash
python docs/fairness_hidden_like_suite.py run --count 300
```

## Next Priorities (updated 2026-05-14 end-of-day)

### Key insight from 2026-05-14 full-day session
**10 quotas used. 9/10 returned exactly 2.16959.** Only catch-up preemption at 0.05x moved hidden (+0.001). Every other mechanism — steal changes, two-tier selection, half-life tuning, NUMA placement — zero effect on hidden.

The hidden metric is extraordinarily resistant to small mechanism changes. The daemon scenario is the canary — it detects ANY debt signal in preemption paths (on_tick, should_preempt > 0.05x). The safe surface is now fully mapped: window debt in pick_next/steal (no-block gated, 0.5x scale), catch-up preemption at 0.05x, and steal ignoring oversubscription. Nothing else is both safe AND effective.

The hidden-like suite geomean (~1.767) does not correlate with real hidden scores — we're optimizing blind.

### 1. Framework-level changes (only path to 2.30+)
- **Group-level time slices**: First pick which GROUP should run, then pick task within group. Must NOT create the stickiness problem (daemon regression). The key difference from stickiness: time slices have an EXPIRATION, after which the group yields.
- **NUMA-aware steal**: Prefer stealing from same-NUMA-node workers. Current steal ignores topology entirely.
- **Weight-proportional time slice duration**: Groups with higher weight get longer time slices (not shorter quantum — that was dead end #19).
- **Explore h03 specifically**: It's resistant to everything. May need workload-specific analysis.

### 2. Today's findings reinforce
Small tweaks are FULLY EXHAUSTED. Preemption changes (on_tick, should_preempt, min_slice) always regress daemon. Selection biases (starvation, stickiness, two-tier) either regress daemon or have zero effect. Window debt tuning (scale, half-life, min_target) is maxed out. NUMA/placement changes have zero effect. **The only path to 2.30+ is framework-level redesign.**

## Daily Workflow

1. Read this document's "Current State" and "Next Priorities"
2. Make ONE mechanism change, test locally (public + hidden-like)
3. If public doesn't collapse, submit for hidden verification
4. Document all findings here

## Build & Test

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure --parallel 8
python tools/bench.py release --track fairness
python docs/fairness_hidden_like_suite.py run --count 300
```

## Target

- Floor: public fairness `>= 1.90x`, correctness gate pass
- Goal: hidden fairness `2.16959 → 2.30+`
- Primary metric: h03 and h06 must be mechanically lifted, without sacrificing h05
