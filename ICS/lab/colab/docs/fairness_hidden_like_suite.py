#!/usr/bin/env python3
"""Generate and run hidden-like fairness workloads.

Design principles:
- NOT copies of public workloads — designed from scorer mechanics.
- The fairness_share_skew scorer uses a runnable-window target/actual mechanism:
  for each window, per-group target = weight_share * total_service * dt,
  actual = running_count * dt. Skew = max(actual/target, target/actual).
  Final score = 1 / 90th_percentile_window_skew.
- Families target patterns absent from public workloads:
  extreme weight ratios (>=6:1), topology churn, phase transitions,
  cyclic reactivation, window-decay timing races, asymmetric blocking.
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import shutil
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence

REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CASE_DIR = REPO_ROOT / "docs" / "generated" / "fairness_hidden_like"
DEFAULT_RESULTS = REPO_ROOT / "docs" / "generated" / "fairness_hidden_like_results.tsv"
DEFAULT_INSTALL_NAME = "fairness_hidden_like_generated"


def install_dir_for(name: str) -> Path:
    safe = "".join(ch for ch in name if ch.isalnum() or ch in "_-")
    if not safe:
        raise ValueError("install name must contain at least one safe character")
    return REPO_ROOT / "workloads" / "public" / safe


@dataclass(frozen=True)
class Case:
    name: str
    family: str
    text: str
    note: str


def duration_us(value: int) -> str:
    return f"{max(1, value)}us"


def header(
    workers: int,
    cpu_rate: int,
    switch_cost: int,
    migration_cost: int,
    weight: float = 1.0,
    topology: bool = False,
) -> list[str]:
    lines = [
        "track fairness",
        "score fairness_share_skew",
        "role leaderboard",
        f"scenario_weight = {weight:.2f}",
        f"workers {workers}",
        f"cpu_rate {cpu_rate}",
        f"switch_cost {switch_cost}",
    ]
    if topology:
        mid = workers // 2
        lines.extend([
            f"node n0 0-{mid - 1}",
            f"node n1 {mid}-{workers - 1}",
            f"migration_cost local={max(1, migration_cost // 2)} remote={migration_cost * 2}",
        ])
    else:
        lines.append(f"migration_cost {migration_cost}")
    return lines


def block(lines: Iterable[str]) -> str:
    return "\n".join(lines).rstrip() + "\n"


# ---------------------------------------------------------------------------
# Family builders
# ---------------------------------------------------------------------------


def two_group_stagger_no_block(i: int) -> Case:
    """Two groups, staggered arrival, no blocking. Weight ratio varies 3-10."""
    ratios = [3, 4, 5, 6, 8, 10]
    ratio = ratios[i % len(ratios)]
    workers = [4, 6, 8, 10, 12][i % 5]
    high_count = [2, 3, 4, 5, 6][i % 5]
    low_count = high_count * [3, 4, 5, 6, 8][i % 5]
    low_compute = 55 + (i % 9) * 22
    high_compute = 70 + (i % 8) * 28
    stagger = 80 + (i % 11) * 75
    delay = 150 + (i % 7) * 200
    lines = header(workers, 12 + (i % 5) * 4, 2 + (i % 6), 5 + (i % 5) * 2)
    lines.extend([
        "",
        "phase low_first at 0us:",
        f"  group low_bulk * {low_count} weight=1:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {low_compute}",
        "",
        f"phase high_join at {duration_us(delay)}:",
        f"  group high_stagger * {high_count} weight={ratio} arrival stagger {duration_us(stagger)}:",
        f"    repeat {3 + (i % 5)}:",
        f"      compute {high_compute}",
    ])
    return Case(
        f"case_{i:03d}",
        "two_group_stagger_no_block",
        block(lines),
        f"ratio={ratio}, low_count={low_count}, high_count={high_count}",
    )


def inverse_stagger_no_block(i: int) -> Case:
    """Heavy group arrives first, light group arrives later — inverse weight skew."""
    ratios = [4, 5, 7, 9, 12]
    ratio = ratios[i % len(ratios)]
    workers = [5, 7, 9, 12, 16][i % 5]
    high_count = [2, 3, 4, 5][i % 4]
    low_count = high_count * [4, 6, 8, 10][i % 4]
    lines = header(workers, 10 + (i % 6) * 3, 3 + (i % 5), 7 + (i % 6) * 2)
    lines.extend([
        "",
        "phase high_first at 0us:",
        f"  group high_anchor * {high_count} weight={ratio}:",
        f"    repeat {4 + (i % 6)}:",
        f"      compute {85 + (i % 8) * 20}",
        "",
        f"phase low_wave at {duration_us(120 + (i % 9) * 100)}:",
        f"  group low_wave * {low_count} weight=1 arrival burst {2 + (i % 5)} every {duration_us(130 + (i % 7) * 90)}:",
        f"    repeat {3 + (i % 5)}:",
        f"      compute {35 + (i % 10) * 12}",
    ])
    return Case(
        f"case_{i:03d}",
        "inverse_stagger_no_block",
        block(lines),
        f"ratio={ratio}, high_count={high_count}, low_count={low_count}",
    )


def sleepy_reactivation(i: int) -> Case:
    """Groups with sleep cycles — tests reactivation fairness after idle periods."""
    weight_pairs = [(1, 3), (1, 4), (1, 5), (2, 5), (3, 1), (4, 1), (1, 6), (1, 8)]
    steady_w, burst_w = weight_pairs[i % len(weight_pairs)]
    workers = [4, 6, 8, 10][i % 4]
    lines = header(workers, 14 + (i % 5) * 4, 2 + (i % 6), 4 + (i % 7) * 2)
    sleep_a = 120 + (i % 10) * 130
    sleep_b = 400 + (i % 8) * 320
    lines.extend([
        "",
        "phase mixed at 0us:",
        f"  group steady * {6 + (i % 6) * 3} weight={steady_w}:",
        f"    repeat {6 + (i % 6)}:",
        f"      compute {45 + (i % 11) * 10}",
        f"      sleep {duration_us(sleep_a)}",
        f"      compute {28 + (i % 9) * 8}",
        f"  group sleeper * {8 + (i % 7) * 4} weight={burst_w} arrival burst {2 + (i % 5)} every {duration_us(180 + (i % 9) * 140)}:",
        f"    repeat {4 + (i % 6)}:",
        f"      compute {28 + (i % 12) * 9}",
        f"      sleep {duration_us(sleep_b)}",
        f"      compute {20 + (i % 8) * 7}",
    ])
    return Case(
        f"case_{i:03d}",
        "sleepy_reactivation",
        block(lines),
        f"steady_w={steady_w}, sleeper_w={burst_w}",
    )


def device_wakeup(i: int) -> Case:
    """IO device wakeup fairness — one CPU-bound group, one IO-bound group."""
    workers = [4, 6, 8, 10, 12][i % 5]
    io_parallel = [1, 2, 3, 4, 5][i % 5]
    lines = header(workers, 12 + (i % 4) * 5, 3 + (i % 6), 6 + (i % 5) * 2)
    lines.extend([
        f"device io0 fifo rate={6 + (i % 8) * 4} parallel={io_parallel}",
        "",
        "phase io_mix at 0us:",
        f"  group cpu_anchor * {4 + (i % 6)} weight={1 + (i % 4)}:",
        f"    repeat {6 + (i % 6)}:",
        f"      compute {50 + (i % 10) * 14}",
        f"  group io_burst * {8 + (i % 9) * 3} weight={3 + (i % 6)} arrival burst {2 + (i % 5)} every {duration_us(200 + (i % 9) * 100)}:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {20 + (i % 8) * 8}",
        f"      call io0 {28 + (i % 10) * 10}",
        f"      compute {16 + (i % 7) * 6}",
    ])
    return Case(
        f"case_{i:03d}",
        "device_wakeup",
        block(lines),
        f"io_parallel={io_parallel}",
    )


def many_group_microburst(i: int) -> Case:
    """3-5 groups with micro-burst arrivals — tests multi-group fairness tracking."""
    workers = [6, 8, 10, 12, 16][i % 5]
    group_count = 3 + (i % 3)
    lines = header(workers, 16 + (i % 6) * 3, 2 + (i % 7), 5 + (i % 6) * 2)
    lines.extend(["", "phase micro at 0us:"])
    for g in range(group_count):
        weight = [1, 2, 3, 5, 7, 10][(i + g * 2) % 6]
        count = 3 + ((i + g) % 6) * 4
        burst = 1 + ((i + g) % 5)
        every = 50 + ((i + g * 3) % 9) * 55
        lines.extend([
            f"  group g{g} * {count} weight={weight} arrival burst {burst} every {duration_us(every)}:",
            f"    repeat {4 + ((i + g) % 5)}:",
            f"      compute {14 + ((i + g) % 11) * 6}",
        ])
        if (i + g) % 2 == 0:
            lines.append(f"      sleep {duration_us(60 + ((i + g) % 10) * 60)}")
            lines.append(f"      compute {10 + ((i + g) % 8) * 5}")
    return Case(
        f"case_{i:03d}",
        "many_group_microburst",
        block(lines),
        f"groups={group_count}",
    )


def parallelism_mismatch(i: int) -> Case:
    """Asymmetric parallelism — one group has few tasks, another floods all CPUs."""
    workers = [8, 10, 12, 16, 20][i % 5]
    ratios = [4, 6, 8, 10, 15]
    ratio = ratios[i % len(ratios)]
    high_count = [1, 2, 3, 4][i % 4]
    low_count = high_count * [10, 15, 20, 25][i % 4]
    lines = header(workers, 10 + (i % 6) * 4, 4 + (i % 5), 8 + (i % 6) * 2)
    lines.extend([
        "",
        "phase parallel at 0us:",
        f"  group scarce_heavy * {high_count} weight={ratio}:",
        f"    repeat {6 + (i % 6)}:",
        f"      compute {100 + (i % 9) * 24}",
        f"  group abundant_light * {low_count} weight=1 arrival stagger {duration_us(25 + (i % 8) * 22)}:",
        f"    repeat {3 + (i % 5)}:",
        f"      compute {28 + (i % 10) * 10}",
    ])
    return Case(
        f"case_{i:03d}",
        "parallelism_mismatch",
        block(lines),
        f"ratio={ratio}, high_count={high_count}, low_count={low_count}",
    )


def phase_churn(i: int) -> Case:
    """Multi-phase scenario — groups join at different times, testing window adaptation."""
    workers = [4, 6, 8, 12, 16][i % 5]
    lines = header(workers, 12 + (i % 5) * 4, 3 + (i % 6), 6 + (i % 7) * 2)
    t1 = 200 + (i % 9) * 140
    t2 = t1 + 250 + (i % 7) * 200
    lines.extend([
        "",
        "phase base at 0us:",
        f"  group base_a * {4 + (i % 5) * 3} weight={1 + (i % 5)}:",
        f"    repeat {7 + (i % 5)}:",
        f"      compute {42 + (i % 11) * 12}",
        f"  group base_b * {5 + (i % 6) * 3} weight={2 + (i % 6)}:",
        f"    repeat {5 + (i % 6)}:",
        f"      compute {40 + (i % 9) * 14}",
        "",
        f"phase join_one at {duration_us(t1)}:",
        f"  group join_c * {3 + (i % 5)} weight={5 + (i % 5)} arrival stagger {duration_us(60 + (i % 8) * 60)}:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {55 + (i % 8) * 20}",
        "",
        f"phase join_two at {duration_us(t2)}:",
        f"  group join_d * {5 + (i % 7)} weight={1 + (i % 4)} arrival burst {2 + (i % 5)} every {duration_us(110 + (i % 6) * 75)}:",
        f"    repeat {3 + (i % 6)}:",
        f"      compute {30 + (i % 10) * 9}",
    ])
    return Case(
        f"case_{i:03d}",
        "phase_churn",
        block(lines),
        f"t1={t1}, t2={t2}",
    )


def topology_churn(i: int) -> Case:
    """NUMA topology with asymmetric migration costs — tests topology-aware fairness."""
    workers = [8, 10, 12, 16, 20][i % 5]
    lines = header(workers, 14 + (i % 5) * 3, 3 + (i % 6), 6 + (i % 6) * 2, topology=True)
    lines.extend([
        "",
        "phase topo at 0us:",
        f"  group local_pressure * {8 + (i % 7) * 3} weight=1 arrival stagger {duration_us(35 + (i % 9) * 28)}:",
        f"    repeat {4 + (i % 6)}:",
        f"      compute {38 + (i % 9) * 11}",
        f"  group remote_heavy * {2 + (i % 5)} weight={4 + (i % 6)} arrival delay {duration_us(140 + (i % 8) * 130)}:",
        f"    repeat {5 + (i % 6)}:",
        f"      compute {90 + (i % 10) * 18}",
        f"      sleep {duration_us(80 + (i % 7) * 90)}",
    ])
    return Case(
        f"case_{i:03d}",
        "topology_churn",
        block(lines),
        f"workers={workers}",
    )


# ---------------------------------------------------------------------------
# NEW families targeting hidden-like gaps (h03/h06 patterns)
# ---------------------------------------------------------------------------


def late_heavyweight(i: int) -> Case:
    """Heavyweight group arrives late when light groups saturated all CPUs.

    Tests whether the scheduler redistributes CPU when the active weight mix
    changes dramatically — a core window-debt challenge.
    """
    ratios = [6, 8, 10, 12, 16]
    ratio = ratios[i % len(ratios)]
    workers = [6, 8, 10, 12][i % 4]
    light_count = workers * [3, 4, 5][i % 3]
    heavy_count = max(1, workers // [2, 3, 4][i % 3])
    delay = 300 + (i % 10) * 200
    lines = header(workers, 14 + (i % 5) * 3, 3 + (i % 5), 6 + (i % 6) * 2)
    lines.extend([
        "",
        "phase light_flood at 0us:",
        f"  group light_flood * {light_count} weight=1 arrival burst {workers} every 10us:",
        f"    repeat {6 + (i % 6)}:",
        f"      compute {40 + (i % 9) * 10}",
        "",
        f"phase heavy_arrives at {duration_us(delay)}:",
        f"  group heavy_late * {heavy_count} weight={ratio} arrival stagger {duration_us(30 + (i % 8) * 25)}:",
        f"    repeat {5 + (i % 5)}:",
        f"      compute {80 + (i % 10) * 22}",
    ])
    return Case(
        f"case_{i:03d}",
        "late_heavyweight",
        block(lines),
        f"ratio={ratio}, delay={delay}",
    )


def cyclic_reactivation(i: int) -> Case:
    """Groups that repeatedly block and return — tests reactivation clamp under cycles.

    Multiple block/wakeup cycles challenge the reactivation mechanism:
    too aggressive clamp → starvation; too loose → unfair advantage.
    """
    workers = [4, 6, 8, 10][i % 4]
    weight_pairs = [(1, 4), (1, 6), (1, 8), (4, 1), (6, 1)]
    light_w, heavy_w = weight_pairs[i % len(weight_pairs)]
    cycles = 3 + (i % 5)
    sleep_each = 60 + (i % 10) * 45
    compute_each = 25 + (i % 9) * 12
    lines = header(workers, 16 + (i % 4) * 4, 2 + (i % 6), 5 + (i % 5) * 2)
    lines.extend([
        "",
        "phase cyclic at 0us:",
        f"  group light_cyclic * {6 + (i % 6) * 2} weight={light_w}:",
        f"    repeat {cycles}:",
        f"      compute {compute_each}",
        f"      sleep {duration_us(sleep_each)}",
        f"      compute {compute_each + 10}",
        f"  group heavy_cyclic * {3 + (i % 5)} weight={heavy_w}:",
        f"    repeat {cycles}:",
        f"      compute {compute_each * 2}",
        f"      sleep {duration_us(sleep_each * 3)}",
        f"      compute {compute_each + 15}",
    ])
    return Case(
        f"case_{i:03d}",
        "cyclic_reactivation",
        block(lines),
        f"light_w={light_w}, heavy_w={heavy_w}, cycles={cycles}",
    )


def window_decay_race(i: int) -> Case:
    """Arrival patterns that interact with the scorer's exponential window decay.

    The fairness scorer uses adaptive windows (span / target_window_count) with
    exponential decay in our scheduler's window_debt tracking. This family
    creates arrival patterns that stress the window boundary alignment.
    """
    workers = [4, 6, 8, 10][i % 4]
    ratio = [3, 5, 7, 9][i % 4]
    # Phase 1: one group runs alone, building window dominance
    # Phase 2: second group joins right when window would decay
    phase2_delay = 200 + (i % 8) * 150
    lines = header(workers, 12 + (i % 6) * 3, 3 + (i % 5), 6 + (i % 6))
    lines.extend([
        "",
        "phase solo at 0us:",
        f"  group solo_first * {workers * 2} weight={ratio}:",
        f"    repeat {5 + (i % 6)}:",
        f"      compute {60 + (i % 10) * 16}",
        "",
        f"phase challenger at {duration_us(phase2_delay)}:",
        f"  group challenger * {workers * [3, 4, 5][i % 3]} weight=1 arrival burst {workers} every 5us:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {35 + (i % 10) * 10}",
    ])
    return Case(
        f"case_{i:03d}",
        "window_decay_race",
        block(lines),
        f"ratio={ratio}, phase2_delay={phase2_delay}",
    )


def weight_transition(i: int) -> Case:
    """Active weight mix changes when some groups exit mid-scenario.

    Tests whether the scheduler correctly adapts to a shrinking active set
    where the weight distribution shifts dramatically.
    """
    workers = [6, 8, 10, 12][i % 4]
    lines = header(workers, 14 + (i % 5) * 4, 3 + (i % 6), 7 + (i % 5) * 2)
    # Phase 1: three groups active
    # Phase 2: heaviest group exits, leaving light groups
    lines.extend([
        "",
        "phase three_groups at 0us:",
        f"  group heavy_exit * {2 + (i % 4)} weight={6 + (i % 6)}:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {100 + (i % 8) * 20}",
        f"  group mid_stay * {4 + (i % 5)} weight=2:",
        f"    repeat {5 + (i % 6)}:",
        f"      compute {55 + (i % 9) * 12}",
        f"  group light_stay * {8 + (i % 6)} weight=1:",
        f"    repeat {4 + (i % 5)}:",
        f"      compute {35 + (i % 8) * 8}",
    ])
    return Case(
        f"case_{i:03d}",
        "weight_transition",
        block(lines),
        f"workers={workers}",
    )


def asymmetric_blocking(i: int) -> Case:
    """One group blocks frequently, the other runs continuously.

    Tests whether `group_has_blocked` detection correctly gates window debt
    without starving the non-blocking group or over-penalizing the blocking one.
    """
    workers = [4, 6, 8, 10][i % 4]
    ratios = [3, 5, 7, 10]
    ratio = ratios[i % len(ratios)]
    lines = header(workers, 14 + (i % 5) * 3, 3 + (i % 6), 6 + (i % 5) * 2)
    lines.extend([
        "",
        "phase asym_block at 0us:",
        f"  group continuous * {6 + (i % 6) * 2} weight={ratio}:",
        f"    repeat {8 + (i % 6)}:",
        f"      compute {50 + (i % 10) * 12}",
        f"  group blocking * {workers * 2} weight=1 arrival burst {2 + (i % 4)} every {duration_us(100 + (i % 8) * 60)}:",
        f"    repeat {5 + (i % 5)}:",
        f"      compute {22 + (i % 9) * 7}",
        f"      sleep {duration_us(40 + (i % 10) * 35)}",
        f"      compute {18 + (i % 8) * 6}",
    ])
    return Case(
        f"case_{i:03d}",
        "asymmetric_blocking",
        block(lines),
        f"ratio={ratio}",
    )


def bursty_reactivation_storm(i: int) -> Case:
    """Many short-lived tasks creating rapid runnable count changes.

    Tests whether the scheduler can maintain fairness when the runnable set
    changes faster than the window tracking can stabilize.
    """
    workers = [6, 8, 10, 12][i % 4]
    lines = header(workers, 18 + (i % 3) * 5, 2 + (i % 6), 5 + (i % 5) * 2)
    lines.extend([
        "",
        "phase storm at 0us:",
        f"  group anchor * {4 + (i % 5)} weight={2 + (i % 5)}:",
        f"    repeat {10 + (i % 6)}:",
        f"      compute {30 + (i % 10) * 8}",
        f"  group storm_a * {workers * 3} weight={1 + (i % 2)} arrival burst {workers} every {duration_us(80 + (i % 8) * 50)}:",
        f"    repeat {3 + (i % 5)}:",
        f"      compute {12 + (i % 10) * 4}",
        f"      sleep {duration_us(25 + (i % 8) * 20)}",
        f"  group storm_b * {workers * 2} weight={3 + (i % 4)} arrival burst {max(1, workers // 2)} every {duration_us(110 + (i % 7) * 65)}:",
        f"    repeat {3 + (i % 4)}:",
        f"      compute {15 + (i % 9) * 5}",
        f"      sleep {duration_us(35 + (i % 7) * 30)}",
    ])
    return Case(
        f"case_{i:03d}",
        "bursty_reactivation_storm",
        block(lines),
        f"workers={workers}",
    )


FAMILY_BUILDERS: Sequence = [
    two_group_stagger_no_block,
    inverse_stagger_no_block,
    sleepy_reactivation,
    device_wakeup,
    many_group_microburst,
    parallelism_mismatch,
    phase_churn,
    topology_churn,
    late_heavyweight,
    cyclic_reactivation,
    window_decay_race,
    weight_transition,
    asymmetric_blocking,
    bursty_reactivation_storm,
]


def make_cases(count: int) -> list[Case]:
    cases: list[Case] = []
    for i in range(count):
        builder = FAMILY_BUILDERS[i % len(FAMILY_BUILDERS)]
        cases.append(builder(i))
    return cases


# ---------------------------------------------------------------------------
# File I/O
# ---------------------------------------------------------------------------


def generate_cases(case_dir: Path, count: int) -> list[Case]:
    cases = make_cases(count)
    if case_dir.exists():
        shutil.rmtree(case_dir)
    case_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = case_dir / "manifest.tsv"
    with manifest_path.open("w", newline="") as manifest_file:
        writer = csv.writer(manifest_file, delimiter="\t")
        writer.writerow(["name", "family", "note"])
        for case in cases:
            (case_dir / f"{case.name}.sched").write_text(case.text, encoding="utf-8")
            writer.writerow([case.name, case.family, case.note])
    readme = case_dir / "README.md"
    readme.write_text(
        "# Fairness Hidden-Like Generated Suite\n\n"
        "Generated by `docs/fairness_hidden_like_suite.py`.\n\n"
        "14 families targeting patterns absent from public workloads: "
        "extreme weight ratios, topology churn, phase transitions, "
        "cyclic reactivation, window-decay races, asymmetric blocking, "
        "and bursty reactivation storms.\n",
        encoding="utf-8",
    )
    return cases


def remove_tree(path: Path) -> None:
    for _attempt in range(5):
        if not path.exists():
            return
        try:
            shutil.rmtree(path)
            return
        except OSError:
            if _attempt == 4:
                raise
            time.sleep(0.1)


def install_cases(case_dir: Path, install_name: str) -> None:
    install_dir = install_dir_for(install_name)
    remove_tree(install_dir)
    install_dir.parent.mkdir(parents=True, exist_ok=True)
    shutil.copytree(case_dir, install_dir)


def clean_installed(install_name: str) -> None:
    remove_tree(install_dir_for(install_name))


def scenario_id_for(case: Case, install_name: str) -> str:
    return f"public/{install_dir_for(install_name).name}/{case.name}"


# ---------------------------------------------------------------------------
# Runner
# ---------------------------------------------------------------------------


def runner_env() -> dict[str, str]:
    env = dict(os.environ)
    runner = REPO_ROOT / "build" / "benchmark" / "runner"
    if "SCHEDLAB_RUNNER" not in env and runner.exists():
        env["SCHEDLAB_RUNNER"] = str(runner)
    return env


def run_one(case: Case, repetitions: int, install_name: str) -> dict:
    tools_dir = REPO_ROOT / "tools"
    if str(tools_dir) not in sys.path:
        sys.path.insert(0, str(tools_dir))
    import benchlib

    records = benchlib.collect_runner_records(
        REPO_ROOT,
        [
            "--mode", "release",
            "--engine", "sim",
            "--suite", "public",
            "--scenario", scenario_id_for(case, install_name),
            "--repetitions", str(repetitions),
        ],
        env=runner_env(),
    )
    scored = next(record for record in records if record.get("type") == "scenario_scored")
    student_fairness = scored.get("student_fairness", {}) or {}
    groups = student_fairness.get("groups", []) or []
    worst_group = ""
    worst_distortion = 0.0
    for group in groups:
        distortion = float(group.get("share_distortion", 0.0) or 0.0)
        severity = max(distortion, 1.0 / distortion) if distortion > 0.0 else 1000.0
        if severity > worst_distortion:
            worst_distortion = severity
            worst_group = str(group.get("group_name", ""))
    return {
        "name": case.name,
        "family": case.family,
        "score": float(scored.get("score", 0.0) or 0.0),
        "student_quality": float(scored.get("student_quality", 0.0) or 0.0),
        "baseline_quality": float(scored.get("baseline_quality", 0.0) or 0.0),
        "max_share_skew": float(student_fairness.get("max_share_skew", 0.0) or 0.0),
        "share_balance_ratio": float(student_fairness.get("share_balance_ratio", 0.0) or 0.0),
        "worst_group": worst_group,
        "worst_group_severity": worst_distortion,
        "note": case.note,
    }


# ---------------------------------------------------------------------------
# Analysis
# ---------------------------------------------------------------------------


def geometric_mean(values: Iterable[float]) -> float:
    vals = [value for value in values if value > 0.0]
    if not vals:
        return 0.0
    return math.exp(sum(math.log(value) for value in vals) / len(vals))


def write_results(results: list[dict], output: Path) -> None:
    output.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "name", "family", "score", "student_quality", "baseline_quality",
        "max_share_skew", "share_balance_ratio", "worst_group",
        "worst_group_severity", "note",
    ]
    with output.open("w", newline="") as output_file:
        writer = csv.DictWriter(output_file, delimiter="\t", fieldnames=fields)
        writer.writeheader()
        for row in sorted(results, key=lambda item: item["score"]):
            formatted = dict(row)
            for key in [
                "score", "student_quality", "baseline_quality",
                "max_share_skew", "share_balance_ratio", "worst_group_severity",
            ]:
                formatted[key] = f"{float(formatted[key]):.6f}"
            writer.writerow(formatted)


def summarize(results: list[dict]) -> str:
    lines: list[str] = []
    lines.append(f"cases={len(results)}")
    lines.append(f"geomean_score={geometric_mean(row['score'] for row in results):.6f}")
    lines.append(f"min_score={min(row['score'] for row in results):.6f}")
    sorted_scores = sorted(row["score"] for row in results)
    lines.append(f"median_score={sorted_scores[len(results) // 2]:.6f}")
    lines.append("")
    lines.append("family_geomean:")
    for family in sorted({row["family"] for row in results}):
        family_rows = [row for row in results if row["family"] == family]
        lines.append(f"  {family}: {geometric_mean(row['score'] for row in family_rows):.6f}")
    lines.append("")
    lines.append("worst_15:")
    for row in sorted(results, key=lambda item: item["score"])[:15]:
        lines.append(
            f"  {row['name']} {row['family']} score={row['score']:.6f} "
            f"skew={row['max_share_skew']:.3f} worst={row['worst_group']}"
        )
    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------


def cmd_generate(args: argparse.Namespace) -> None:
    cases = generate_cases(args.case_dir, args.count)
    print(f"generated {len(cases)} cases in {args.case_dir}")


def cmd_run(args: argparse.Namespace) -> None:
    cases = generate_cases(args.case_dir, args.count)
    install_cases(args.case_dir, args.install_name)
    try:
        selected = cases[: args.limit] if args.limit else cases
        results: list[dict] = []
        for index, case in enumerate(selected, start=1):
            result = run_one(case, args.repetitions, args.install_name)
            results.append(result)
            if args.verbose or index % 10 == 0 or index == len(selected):
                print(
                    f"[{index:03d}/{len(selected):03d}] {case.name} {case.family} "
                    f"score={result['score']:.6f}"
                )
        write_results(results, args.output)
        summary = summarize(results)
        summary_path = args.output.with_suffix(".summary.txt")
        summary_path.write_text(summary + "\n", encoding="utf-8")
        print(summary)
        print(f"wrote {args.output}")
        print(f"wrote {summary_path}")
    finally:
        if not args.keep_installed:
            clean_installed(args.install_name)


def cmd_clean(args: argparse.Namespace) -> None:
    install_dir = install_dir_for(args.install_name)
    clean_installed(args.install_name)
    print(f"removed {install_dir}")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate and run hidden-like fairness workloads."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate = subparsers.add_parser("generate")
    generate.add_argument("--count", type=int, default=300)
    generate.add_argument("--case-dir", type=Path, default=DEFAULT_CASE_DIR)
    generate.set_defaults(func=cmd_generate)

    run = subparsers.add_parser("run")
    run.add_argument("--count", type=int, default=300)
    run.add_argument("--limit", type=int, default=0, help="Run only the first N cases.")
    run.add_argument("--repetitions", type=int, default=1)
    run.add_argument("--case-dir", type=Path, default=DEFAULT_CASE_DIR)
    run.add_argument("--output", type=Path, default=DEFAULT_RESULTS)
    run.add_argument("--install-name", default=DEFAULT_INSTALL_NAME)
    run.add_argument("--keep-installed", action="store_true")
    run.add_argument("--verbose", action="store_true")
    run.set_defaults(func=cmd_run)

    clean = subparsers.add_parser("clean-installed")
    clean.add_argument("--install-name", default=DEFAULT_INSTALL_NAME)
    clean.set_defaults(func=cmd_clean)

    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    if getattr(args, "count", 1) <= 0:
        parser.error("--count must be positive")
    args.func(args)


if __name__ == "__main__":
    main()
