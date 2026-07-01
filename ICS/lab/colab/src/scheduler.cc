#include "scheduler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <unordered_map>
#include <unordered_set>

namespace student {

namespace {

// ---------------------------------------------------------------------------
// Trace infrastructure: shadow-compute whether window debt changes decisions
// ---------------------------------------------------------------------------
struct TraceCounters {
  uint64_t pick_total = 0;
  uint64_t pick_debt_changes = 0;        // debt changed the chosen task
  uint64_t pick_debt_available = 0;      // eligible for broad debt but gate off
  uint64_t pick_debt_available_changes = 0; // eligible AND would change decision

  uint64_t steal_total = 0;
  uint64_t steal_debt_changes = 0;
  uint64_t steal_debt_available = 0;
  uint64_t steal_debt_available_changes = 0;

  // Category breakdown for "eligible but gate off, would change"
  uint64_t cat_2g_4_1_no_block = 0;        // 4:1, 2 groups, no block
  uint64_t cat_2g_ge5_1_no_block = 0;      // >=5:1, 2 groups, no block
  uint64_t cat_2g_other_no_block = 0;      // other ratio, 2 groups, no block
  uint64_t cat_multigrp_no_block = 0;      // >2 groups, no block
  uint64_t cat_has_blocked = 0;            // any blocking present
};

TraceCounters& trace() {
  static TraceCounters tc;
  return tc;
}

bool trace_enabled() {
  static const bool enabled = []() {
    const char* v = std::getenv("SCHED_FAIR_TRACE");
    return v != nullptr && v[0] == '1' && v[1] == '\0';
  }();
  return enabled;
}

void trace_print() {
  if (!trace_enabled()) return;
  const auto& t = trace();
  std::fprintf(stderr,
      "SCHED_TRACE pick: total=%lu debt_changes=%lu available=%lu available_changes=%lu\n",
      (unsigned long)t.pick_total, (unsigned long)t.pick_debt_changes,
      (unsigned long)t.pick_debt_available, (unsigned long)t.pick_debt_available_changes);
  std::fprintf(stderr,
      "SCHED_TRACE steal: total=%lu debt_changes=%lu available=%lu available_changes=%lu\n",
      (unsigned long)t.steal_total, (unsigned long)t.steal_debt_changes,
      (unsigned long)t.steal_debt_available, (unsigned long)t.steal_debt_available_changes);
  std::fprintf(stderr,
      "SCHED_TRACE cats: 4_1_noblock=%lu ge5_1_noblock=%lu other_noblock=%lu "
      "multigrp_noblock=%lu has_blocked=%lu\n",
      (unsigned long)t.cat_2g_4_1_no_block, (unsigned long)t.cat_2g_ge5_1_no_block,
      (unsigned long)t.cat_2g_other_no_block, (unsigned long)t.cat_multigrp_no_block,
      (unsigned long)t.cat_has_blocked);
}

struct TraceAtExit {
  ~TraceAtExit() { trace_print(); }
};

// Register atexit via static initializer — runs once per process.
static TraceAtExit trace_atexit;

// Helper to compute group_score WITHOUT window debt, for shadow comparison.
// This is the same as group_score but with debt_key always 0.
// We pass all the necessary captured state explicitly.

constexpr uint64_t kDefaultMinQuantumUs = 24;
constexpr uint64_t kDefaultMaxQuantumUs = 80;
constexpr uint64_t kDefaultMinSliceBeforePreemptUs = 8;
constexpr uint64_t kDefaultQuantumWeightBonusUs = 2;
constexpr uint64_t kDefaultReactivationBurstCreditUs = 0;
constexpr uint64_t kDefaultReactivationBurstEpochs = 6;
constexpr uint64_t kDefaultReactivationIdleUs = 1000;
constexpr uint64_t kDefaultMultigroupReactivationBurstCreditUs = 24;
constexpr uint64_t kDefaultWindowDebtHalfLifeUs = 2000;
constexpr uint64_t kDefaultWindowDebtCapUs = 96;
constexpr uint64_t kDefaultLowWeightWindowDebtMinTargetUs = 24;
constexpr uint64_t kDefaultWindowReactivationFloorUs = 0;

constexpr double kDefaultAdmissionSlack = 1.0;
constexpr double kDefaultWindowDebtScale = 1.0;
constexpr double kDefaultLowWeightWindowDebtScale = 1.0;
constexpr double kDefaultLowWeightWindowDebtPositiveRatio = 1.0;
constexpr double kDefaultWindowDebtWeight = 10.0;
constexpr double kDefaultWindowDebtMinTargetTotalUs = 1000.0;
constexpr uint64_t kDefaultWindowDebtBroadMaxRatio = 8;
constexpr bool kDefaultWindowDebtBroadEnabled = true;
constexpr double kDefaultQuantumSwitchScale = 7.0;
constexpr double kDefaultWakeupPreemptSlack = 4.0;
constexpr double kDefaultTickPreemptSlack = 6.0;
constexpr double kDefaultParallelismSlack = 1.0;
constexpr double kDefaultOversubPenaltyScale = 1.0;
constexpr double kDefaultReactivationWakeupSlackScale = 0.85;

constexpr uint32_t kDefaultWakeupLocalityLoadSlack = 1;

uint64_t env_u64(const char* name, uint64_t fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const unsigned long long parsed = std::strtoull(value, &end, 10);
  if (end == value || *end != '\0') {
    return fallback;
  }
  return static_cast<uint64_t>(parsed);
}

double env_double(const char* name, double fallback) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return fallback;
  }
  char* end = nullptr;
  const double parsed = std::strtod(value, &end);
  if (end == value || *end != '\0') {
    return fallback;
  }
  return parsed;
}

uint64_t cfg_min_quantum_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_MIN_QUANTUM_US", kDefaultMinQuantumUs);
  return v;
}

uint64_t cfg_max_quantum_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_MAX_QUANTUM_US", kDefaultMaxQuantumUs);
  return v;
}

uint64_t cfg_quantum_weight_bonus_us() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_QUANTUM_WEIGHT_BONUS_US", kDefaultQuantumWeightBonusUs);
  return v;
}

double cfg_quantum_switch_scale() {
  static const double v = env_double("SCHED_FAIR_QUANTUM_SWITCH_SCALE", kDefaultQuantumSwitchScale);
  return v;
}

uint64_t cfg_min_slice_before_preempt_us() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_MIN_SLICE_PREEMPT_US", kDefaultMinSliceBeforePreemptUs);
  return v;
}

double cfg_wakeup_preempt_slack() {
  static const double v = env_double("SCHED_FAIR_WAKEUP_PREEMPT_SLACK", kDefaultWakeupPreemptSlack);
  return v;
}

double cfg_tick_preempt_slack() {
  static const double v = env_double("SCHED_FAIR_TICK_PREEMPT_SLACK", kDefaultTickPreemptSlack);
  return v;
}

double cfg_parallelism_slack() {
  static const double v = env_double("SCHED_FAIR_PARALLELISM_SLACK", kDefaultParallelismSlack);
  return v;
}

double cfg_oversub_penalty_scale() {
  static const double v =
      env_double("SCHED_FAIR_OVERSUB_PENALTY_SCALE", kDefaultOversubPenaltyScale);
  return v;
}

uint32_t cfg_wakeup_locality_load_slack() {
  static const uint32_t v = static_cast<uint32_t>(
      env_u64("SCHED_FAIR_WAKEUP_LOCALITY_LOAD_SLACK", kDefaultWakeupLocalityLoadSlack));
  return v;
}

uint64_t cfg_reactivation_burst_credit_us() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_REACTIVATION_BURST_US", kDefaultReactivationBurstCreditUs);
  return v;
}

uint64_t cfg_multigroup_reactivation_burst_credit_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_MULTIGROUP_REACTIVATION_BURST_US",
                                    kDefaultMultigroupReactivationBurstCreditUs);
  return v;
}

uint64_t cfg_reactivation_burst_epochs() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_REACTIVATION_BURST_EPOCHS", kDefaultReactivationBurstEpochs);
  return v;
}

double cfg_reactivation_wakeup_slack_scale() {
  static const double v =
      env_double("SCHED_FAIR_REACTIVATION_WAKEUP_SCALE", kDefaultReactivationWakeupSlackScale);
  return v;
}

uint64_t cfg_reactivation_idle_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_REACTIVATION_IDLE_US", kDefaultReactivationIdleUs);
  return v;
}

double cfg_admission_slack() {
  static const double v = env_double("SCHED_FAIR_ADMISSION_SLACK", kDefaultAdmissionSlack);
  return v;
}

uint64_t cfg_window_debt_half_life_us() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_WINDOW_DEBT_HALF_LIFE_US", kDefaultWindowDebtHalfLifeUs);
  return v;
}

uint64_t cfg_window_debt_cap_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_WINDOW_DEBT_CAP_US", kDefaultWindowDebtCapUs);
  return v;
}

uint64_t cfg_window_reactivation_floor_us() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_WINDOW_REACTIVATION_FLOOR_US", kDefaultWindowReactivationFloorUs);
  return v;
}

double cfg_window_debt_scale() {
  static const double v = env_double("SCHED_FAIR_WINDOW_DEBT_SCALE", kDefaultWindowDebtScale);
  return v;
}

double cfg_window_debt_weight() {
  static const double v =
      env_double("SCHED_FAIR_WINDOW_DEBT_WEIGHT", kDefaultWindowDebtWeight);
  return v;
}

double cfg_window_debt_min_target_total_us() {
  static const double v =
      env_double("SCHED_FAIR_WINDOW_DEBT_MIN_TARGET_TOTAL_US", kDefaultWindowDebtMinTargetTotalUs);
  return v;
}

uint64_t cfg_window_debt_broad_max_ratio() {
  static const uint64_t v =
      env_u64("SCHED_FAIR_WINDOW_DEBT_BROAD_MAX_RATIO", kDefaultWindowDebtBroadMaxRatio);
  return v;
}

bool cfg_window_debt_broad_enabled() {
  static const bool v = []() {
    const char* val = std::getenv("SCHED_FAIR_WINDOW_DEBT_BROAD");
    return val != nullptr && val[0] == '1' && val[1] == '\0';
  }();
  return v;
}

double cfg_low_weight_window_debt_scale() {
  static const double v =
      env_double("SCHED_FAIR_LOW_WEIGHT_WINDOW_DEBT_SCALE", kDefaultLowWeightWindowDebtScale);
  return v;
}

double cfg_low_weight_window_debt_positive_ratio() {
  static const double v = env_double("SCHED_FAIR_LOW_WEIGHT_WINDOW_DEBT_POSITIVE_RATIO",
                                     kDefaultLowWeightWindowDebtPositiveRatio);
  return v;
}

uint64_t cfg_low_weight_window_debt_min_target_us() {
  static const uint64_t v = env_u64("SCHED_FAIR_LOW_WEIGHT_WINDOW_DEBT_MIN_TARGET_US",
                                    kDefaultLowWeightWindowDebtMinTargetUs);
  return v;
}

} // namespace

uint64_t Scheduler::normalized_weight(uint64_t weight) const {
  return std::max<uint64_t>(weight, 1);
}

uint64_t Scheduler::quantum_for_weight(const schedlab::SystemView& system, uint64_t weight) const {
  const uint64_t w = normalized_weight(weight);
  const uint64_t min_q = cfg_min_quantum_us();
  const uint64_t max_q = std::max<uint64_t>(min_q, cfg_max_quantum_us());
  const uint64_t switch_scaled = static_cast<uint64_t>(
      std::llround(static_cast<double>(system.switch_cost()) * cfg_quantum_switch_scale()));
  const uint64_t base = std::max<uint64_t>(min_q, switch_scaled);
  const uint64_t boosted = base + ((w - 1) * cfg_quantum_weight_bonus_us());
  return std::min<uint64_t>(max_q, boosted);
}

void Scheduler::remember_task_locked(const schedlab::TaskView& task) {
  group_weight_[task.group_id] = normalized_weight(task.weight);
  (void)group_service_us_.try_emplace(task.group_id, 0);
  (void)group_boost_us_.try_emplace(task.group_id, 0);
  if (task.total_blocked_time_us > 0 || task.voluntary_block_count > 0) {
    group_has_blocked_[task.group_id] = true;
  } else {
    (void)group_has_blocked_.try_emplace(task.group_id, false);
  }
  (void)task_accounted_runtime_us_.try_emplace(task.task_id, task.total_runtime_us);

  const auto [it, inserted] =
      task_last_blocked_us_.try_emplace(task.task_id, task.total_blocked_time_us);
  if (!inserted && task.total_blocked_time_us > it->second) {
    const uint64_t delta = task.total_blocked_time_us - it->second;
    it->second = task.total_blocked_time_us;
    uint64_t& blocked_max = group_recent_blocked_us_[task.group_id];
    blocked_max = std::max(blocked_max, delta);
  }
}

void Scheduler::account_runtime_locked(const schedlab::TaskView& task) {
  remember_task_locked(task);
  uint64_t& accounted = task_accounted_runtime_us_[task.task_id];
  if (task.total_runtime_us <= accounted) {
    return;
  }
  const uint64_t delta_us = task.total_runtime_us - accounted;
  accounted = task.total_runtime_us;
  group_service_us_[task.group_id] += delta_us;
  account_window_runtime_locked(task.group_id, delta_us);
  uint64_t& boost_us = group_boost_us_[task.group_id];
  if (boost_us > delta_us) {
    boost_us -= delta_us;
  } else {
    boost_us = 0;
  }
}

void Scheduler::account_window_runtime_locked(uint64_t group_id, uint64_t delta_us) {
  if (delta_us == 0) {
    return;
  }

  const double half_life =
      std::max<double>(1.0, static_cast<double>(cfg_window_debt_half_life_us()));
  const double decay = std::exp(-static_cast<double>(delta_us) / half_life);
  for (auto& [tracked_group_id, actual] : group_window_actual_us_) {
    (void)tracked_group_id;
    actual *= decay;
  }
  for (auto& [tracked_group_id, target] : group_window_target_us_) {
    (void)tracked_group_id;
    target *= decay;
  }
  window_target_total_us_ *= decay;

  group_window_actual_us_[group_id] += static_cast<double>(delta_us);

  if (last_active_total_weight_ == 0 || last_active_groups_.empty()) {
    return;
  }
  for (const uint64_t active_group_id : last_active_groups_) {
    const auto weight_it = group_weight_.find(active_group_id);
    const uint64_t weight =
        normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
    const double share =
        static_cast<double>(weight) / static_cast<double>(last_active_total_weight_);
    group_window_target_us_[active_group_id] += static_cast<double>(delta_us) * share;
  }
  window_target_total_us_ += static_cast<double>(delta_us);
}

double Scheduler::fair_key_locked(uint64_t group_id) const {
  const auto weight_it = group_weight_.find(group_id);
  const uint64_t weight =
      normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);

  const auto service_it = group_service_us_.find(group_id);
  const uint64_t service_us = service_it == group_service_us_.end() ? 0 : service_it->second;
  const auto boost_it = group_boost_us_.find(group_id);
  const uint64_t boost_us = boost_it == group_boost_us_.end() ? 0 : boost_it->second;
  const uint64_t effective_service = service_us > boost_us ? (service_us - boost_us) : 0;

  return static_cast<double>(effective_service) / static_cast<double>(weight);
}

double Scheduler::window_debt_key_locked(uint64_t group_id) const {
  const double scale = cfg_window_debt_scale();
  if (scale <= 0.0 || window_target_total_us_ <= 0.0) {
    return 0.0;
  }

  const auto weight_it = group_weight_.find(group_id);
  const uint64_t weight =
      normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
  const auto actual_it = group_window_actual_us_.find(group_id);
  const auto target_it = group_window_target_us_.find(group_id);
  const double actual = actual_it == group_window_actual_us_.end() ? 0.0 : actual_it->second;
  const double target = target_it == group_window_target_us_.end() ? 0.0 : target_it->second;
  if (target <= 0.0) {
    return 0.0;
  }

  const double diff = (actual - target) / static_cast<double>(weight);
  const double cap = std::max<double>(0.0, static_cast<double>(cfg_window_debt_cap_us()));
  return std::clamp(diff * scale, -cap, cap);
}

double Scheduler::window_selection_debt_key_locked(uint64_t group_id,
                                                   uint64_t min_active_weight,
                                                   uint64_t max_active_weight) const {
  const auto weight_it = group_weight_.find(group_id);
  const uint64_t weight =
      normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
  const double debt_key = window_debt_key_locked(group_id);
  if (weight == max_active_weight) {
    return debt_key;
  }
  if (weight != min_active_weight) {
    return 0.0;
  }
  if (debt_key <= 0.0) {
    return debt_key;
  }

  const auto actual_it = group_window_actual_us_.find(group_id);
  const auto target_it = group_window_target_us_.find(group_id);
  const double actual = actual_it == group_window_actual_us_.end() ? 0.0 : actual_it->second;
  const double target = target_it == group_window_target_us_.end() ? 0.0 : target_it->second;
  if (target < static_cast<double>(cfg_low_weight_window_debt_min_target_us())) {
    return 0.0;
  }
  const double positive_ratio = std::max(1.0, cfg_low_weight_window_debt_positive_ratio());
  if (actual < target * positive_ratio) {
    return 0.0;
  }

  return debt_key * std::max(0.0, cfg_low_weight_window_debt_scale());
}

void Scheduler::refresh_active_groups_locked(const schedlab::SystemView& system,
                                             const std::unordered_set<uint64_t>& active_groups) {
  if (active_groups.empty()) {
    return;
  }

  ++active_epoch_;
  last_active_groups_ = active_groups;
  last_active_total_weight_ = 0;
  double min_key = std::numeric_limits<double>::infinity();
  for (const uint64_t group_id : active_groups) {
    const auto weight_it = group_weight_.find(group_id);
    const uint64_t weight =
        normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
    const auto svc_it = group_service_us_.find(group_id);
    const uint64_t service_us = svc_it == group_service_us_.end() ? 0 : svc_it->second;
    min_key = std::min(min_key, static_cast<double>(service_us) / static_cast<double>(weight));
    last_active_total_weight_ += weight;
  }
  if (!std::isfinite(min_key)) {
    min_key = 0.0;
  }

  const double margin_key = std::max<double>(static_cast<double>(cfg_max_quantum_us()) * 2.0,
                                             static_cast<double>(system.switch_cost()) * 16.0);
  const uint64_t burst_credit_us = cfg_reactivation_burst_credit_us();
  const uint64_t multigroup_burst_credit_us =
      active_groups.size() >= 3 ? cfg_multigroup_reactivation_burst_credit_us() : 0;
  const uint64_t idle_threshold_us = cfg_reactivation_idle_us();
  const uint64_t window_floor_us = cfg_window_reactivation_floor_us();
  const double long_idle_margin_key = std::max<double>(
      static_cast<double>(cfg_min_quantum_us()), static_cast<double>(system.switch_cost()) * 4.0);

  for (const uint64_t group_id : active_groups) {
    const auto last_it = group_last_active_epoch_.find(group_id);
    const bool reactivated =
        last_it == group_last_active_epoch_.end() || active_epoch_ - last_it->second > 3;
    if (reactivated) {
      const auto blocked_it = group_recent_blocked_us_.find(group_id);
      const uint64_t blocked_delta =
          blocked_it == group_recent_blocked_us_.end() ? 0 : blocked_it->second;
      const bool long_idle =
          last_it == group_last_active_epoch_.end() || blocked_delta >= idle_threshold_us;
      const auto weight_it = group_weight_.find(group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      const uint64_t target_service =
          static_cast<uint64_t>(std::llround(min_key * static_cast<double>(weight)));
      const double selected_margin = long_idle ? long_idle_margin_key : margin_key;
      const uint64_t clamp_service =
          target_service +
          static_cast<uint64_t>(std::llround(selected_margin * static_cast<double>(weight)));
      uint64_t& service_us = group_service_us_[group_id];
      if (service_us > clamp_service) {
        service_us = clamp_service;
      }
      const uint64_t base_credit = service_us > target_service ? (service_us - target_service) : 0;
      const bool long_idle_epoch =
          last_it == group_last_active_epoch_.end() ||
          active_epoch_ - last_it->second > cfg_reactivation_burst_epochs();
      const uint64_t selected_burst_credit_us =
          std::max(burst_credit_us, multigroup_burst_credit_us);
      const uint64_t extra_credit =
          long_idle_epoch && weight != 0 &&
                  selected_burst_credit_us > (std::numeric_limits<uint64_t>::max() / weight)
              ? std::numeric_limits<uint64_t>::max()
              : (long_idle_epoch ? (selected_burst_credit_us * weight) : 0);
      const uint64_t boost_us = base_credit > (std::numeric_limits<uint64_t>::max() - extra_credit)
                                    ? std::numeric_limits<uint64_t>::max()
                                    : base_credit + extra_credit;
      group_boost_us_[group_id] = boost_us;

      if (window_floor_us > 0) {
        double& window_actual = group_window_actual_us_[group_id];
        double& window_target = group_window_target_us_[group_id];
        window_actual = std::min(window_actual, window_target);
        const double target_floor =
            static_cast<double>(window_floor_us) * static_cast<double>(weight);
        window_target = std::max(window_target, window_actual + target_floor);
      }
    }
    group_last_active_epoch_[group_id] = active_epoch_;
    group_recent_blocked_us_[group_id] = 0;
  }
}

int Scheduler::choose_least_loaded_worker(const schedlab::SystemView& system,
                                          int preferred_worker) {
  const uint32_t worker_count = system.total_worker_count();
  if (worker_count == 0) {
    return 0;
  }

  const auto states = system.worker_states();
  int best_worker = states.front().worker_id;
  uint32_t best_load = std::numeric_limits<uint32_t>::max();
  int best_topo = -1;

  // Determine preferred NUMA node for locality tiebreaking.
  int preferred_node = -1;
  if (preferred_worker >= 0 && preferred_worker < static_cast<int>(states.size())) {
    preferred_node = states[static_cast<std::size_t>(preferred_worker)].topology_node;
  }

  const std::size_t state_count = states.size();
  const std::size_t start = static_cast<std::size_t>(next_spawn_worker_) % state_count;
  for (std::size_t i = 0; i < state_count; ++i) {
    const auto& state = states[(start + i) % state_count];
    const int worker_id = state.worker_id;
    uint32_t load = state.local_queue_size;
    if (!state.is_idle) {
      ++load;
    }

    bool is_better = false;
    if (load < best_load) {
      is_better = true;
    } else if (load == best_load) {
      // Tiebreak: prefer preferred worker, then same NUMA node.
      if (worker_id == preferred_worker) {
        is_better = true;
      } else if (best_worker != preferred_worker && preferred_node >= 0 &&
                 state.topology_node == preferred_node && best_topo != preferred_node) {
        is_better = true;
      }
    }

    if (is_better) {
      best_load = load;
      best_worker = worker_id;
      best_topo = state.topology_node;
    }
  }

  next_spawn_worker_ = (best_worker + 1) % static_cast<int>(worker_count);
  return best_worker;
}

void Scheduler::init(const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  next_spawn_worker_ = 0;
  active_epoch_ = 0;
  task_accounted_runtime_us_.clear();
  task_last_blocked_us_.clear();
  group_weight_.clear();
  group_service_us_.clear();
  group_boost_us_.clear();
  group_recent_blocked_us_.clear();
  group_has_blocked_.clear();
  group_last_active_epoch_.clear();
  group_window_actual_us_.clear();
  group_window_target_us_.clear();
  last_active_groups_.clear();
  last_active_total_weight_ = 0;
  window_target_total_us_ = 0.0;

  for (const auto& worker : system.worker_states()) {
    if (const auto running = system.running_task(worker.worker_id); running.has_value()) {
      remember_task_locked(*running);
    }
    const schedlab::QueueView& queue = system.queue(worker.worker_id);
    for (const auto& task : queue) {
      remember_task_locked(task);
    }
  }
}

int Scheduler::select_worker(const schedlab::TaskView&, schedlab::ReadyContext ctx,
                             const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  if (system.total_worker_count() == 0) {
    return 0;
  }

  int preferred_worker = -1;
  if (ctx.reason == schedlab::ReadyReason::Wakeup) {
    if (ctx.source_worker_id >= 0) {
      preferred_worker = ctx.source_worker_id;
    } else if (ctx.previous_worker_id >= 0) {
      preferred_worker = ctx.previous_worker_id;
    }
  }

  if (preferred_worker >= 0 && preferred_worker < static_cast<int>(system.total_worker_count())) {
    const auto states = system.worker_states();
    uint32_t min_load = std::numeric_limits<uint32_t>::max();
    for (const auto& state : states) {
      uint32_t load = state.local_queue_size + (state.is_idle ? 0u : 1u);
      min_load = std::min(min_load, load);
    }
    std::optional<uint32_t> preferred_load;
    for (const auto& state : states) {
      if (state.worker_id == preferred_worker) {
        preferred_load = state.local_queue_size + (state.is_idle ? 0u : 1u);
        break;
      }
    }

    // 唤醒路径尽量保留局部性，只有明显更拥挤时才迁移。
    if (preferred_load.has_value() &&
        *preferred_load <= min_load + cfg_wakeup_locality_load_slack()) {
      return preferred_worker;
    }
  }

  return choose_least_loaded_worker(system, preferred_worker);
}

std::optional<uint64_t> Scheduler::pick_next(int, schedlab::QueueView candidates,
                                             const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  if (candidates.empty()) {
    return std::nullopt;
  }

  // 汇总全局活跃组与并行度，用于抑制某一组在多核上瞬时过度并行。
  std::unordered_map<uint64_t, uint32_t> running_by_group;
  std::unordered_map<uint64_t, uint32_t> runnable_by_group;
  std::unordered_set<uint64_t> active_groups;
  uint32_t total_running = 0;
  for (const auto& worker : system.worker_states()) {
    if (const auto running = system.running_task(worker.worker_id); running.has_value()) {
      remember_task_locked(*running);
      active_groups.insert(running->group_id);
      ++running_by_group[running->group_id];
      ++runnable_by_group[running->group_id];
      ++total_running;
    }
    const schedlab::QueueView& queue = system.queue(worker.worker_id);
    for (const auto& task : queue) {
      remember_task_locked(task);
      active_groups.insert(task.group_id);
      ++runnable_by_group[task.group_id];
    }
  }

  refresh_active_groups_locked(system, active_groups);

  uint64_t total_active_weight = 0;
  bool active_has_blocked = false;
  uint64_t min_active_weight = std::numeric_limits<uint64_t>::max();
  uint64_t max_active_weight = 0;
  for (const uint64_t group_id : active_groups) {
    const auto it = group_weight_.find(group_id);
    const uint64_t weight = normalized_weight(it == group_weight_.end() ? 1 : it->second);
    total_active_weight += weight;
    min_active_weight = std::min(min_active_weight, weight);
    max_active_weight = std::max(max_active_weight, weight);
    const auto blocked_it = group_has_blocked_.find(group_id);
    if (blocked_it != group_has_blocked_.end() && blocked_it->second) {
      active_has_blocked = true;
    }
  }
  const bool window_debt_narrow = active_groups.size() == 2 && !active_has_blocked &&
                                   min_active_weight > 0 &&
                                   max_active_weight == (4 * min_active_weight);

  const bool window_debt_broad =
      cfg_window_debt_broad_enabled() &&
      active_groups.size() >= 2 &&
      window_target_total_us_ >= cfg_window_debt_min_target_total_us() &&
      max_active_weight <= (cfg_window_debt_broad_max_ratio() * min_active_weight) &&
      !window_debt_narrow;

  const uint32_t total_workers = std::max<uint32_t>(1, system.total_worker_count());
  const double oversub_penalty =
      std::max(12.0, static_cast<double>(system.switch_cost()) * cfg_oversub_penalty_scale());
  const double parallelism_slack = std::max(0.0, cfg_parallelism_slack());

  auto compute_debt_key = [&](uint64_t group_id) -> double {
    if (window_debt_narrow) {
      return window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
    }
    if (window_debt_broad) {
      double debt = window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
      if (active_has_blocked) {
        if (debt >= -3.0) return 0.0;
        return debt;
      }
      return debt * 0.5;
    }
    return 0.0;
  };

  auto group_score = [&](uint64_t group_id) {
    const double debt_key = compute_debt_key(group_id);
    const auto it = group_weight_.find(group_id);
    const uint64_t weight = normalized_weight(it == group_weight_.end() ? 1 : it->second);
    const double fair_key = fair_key_locked(group_id) + debt_key;
    if (total_active_weight == 0) {
      return fair_key;
    }
    const double expected_parallelism =
        std::max(1.0, (static_cast<double>(total_workers) * static_cast<double>(weight)) /
                          static_cast<double>(total_active_weight));
    const double running_parallelism = static_cast<double>(running_by_group[group_id]);
    const double oversub =
        std::max(0.0, running_parallelism - expected_parallelism - parallelism_slack);
    // Weight-adaptive penalty: light groups face stronger deterrent against
    // oversubscription; heavy groups can temporarily oversub to catch up.
    const double weight_penalty_ratio =
        static_cast<double>(total_active_weight) / static_cast<double>(weight);
    const double group_penalty = oversub_penalty * weight_penalty_ratio;
    return fair_key + (oversub * group_penalty);
  };

  // Shadow: compute what broad debt would choose when eligible but not active.
  const bool broad_debt_eligible = active_groups.size() == 2 && !active_has_blocked &&
                                    min_active_weight > 0 &&
                                    max_active_weight >= (4 * min_active_weight) &&
                                    !window_debt_narrow && !window_debt_broad;

  auto group_score_broad_debt = [&](uint64_t group_id) {
    const double debt_key =
        window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
    const auto it = group_weight_.find(group_id);
    const uint64_t weight = normalized_weight(it == group_weight_.end() ? 1 : it->second);
    const double fair_key = fair_key_locked(group_id) + debt_key;
    if (total_active_weight == 0) {
      return fair_key;
    }
    const double expected_parallelism =
        std::max(1.0, (static_cast<double>(total_workers) * static_cast<double>(weight)) /
                          static_cast<double>(total_active_weight));
    const double running_parallelism = static_cast<double>(running_by_group[group_id]);
    const double oversub =
        std::max(0.0, running_parallelism - expected_parallelism - parallelism_slack);
    return fair_key + (oversub * oversub_penalty);
  };

  auto admission_enabled = [&]() {
    if (active_groups.size() != 2 || total_active_weight == 0) {
      return false;
    }
    uint64_t min_weight = std::numeric_limits<uint64_t>::max();
    uint64_t max_weight = 0;
    for (const uint64_t group_id : active_groups) {
      const auto blocked_it = group_has_blocked_.find(group_id);
      if (blocked_it != group_has_blocked_.end() && blocked_it->second) {
        return false;
      }
      const auto weight_it = group_weight_.find(group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      min_weight = std::min(min_weight, weight);
      max_weight = std::max(max_weight, weight);
    }
    return min_weight > 0 && max_weight == (4 * min_weight);
  }();

  auto admission_allows = [&](uint64_t group_id) {
    if (!admission_enabled || total_running == 0) {
      return true;
    }

    double desired_total_running = static_cast<double>(total_workers);
    for (const uint64_t active_group_id : active_groups) {
      const auto weight_it = group_weight_.find(active_group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
      if (share <= 0.0) {
        continue;
      }
      desired_total_running = std::min(
          desired_total_running, static_cast<double>(runnable_by_group[active_group_id]) / share);
    }
    desired_total_running =
        std::clamp(desired_total_running, 1.0, static_cast<double>(total_workers));

    const auto weight_it = group_weight_.find(group_id);
    const uint64_t weight =
        normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
    const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
    const double desired_slots = share * desired_total_running;
    return static_cast<double>(running_by_group[group_id] + 1) <=
           desired_slots + std::max(0.0, cfg_admission_slack());
  };

  std::optional<uint64_t> best_task_id;
  double best_score = std::numeric_limits<double>::max();
  uint64_t best_runtime = std::numeric_limits<uint64_t>::max();

  // Two-tier: track deeply underserved groups (debt < -5.0) for priority promotion.
  std::optional<uint64_t> tier1_task_id;
  double tier1_score = std::numeric_limits<double>::max();
  uint64_t tier1_runtime = std::numeric_limits<uint64_t>::max();

  // Shadow: track what broad debt would choose (trace only).
  std::optional<uint64_t> shadow_broad_task_id;
  double shadow_broad_score = std::numeric_limits<double>::max();
  uint64_t shadow_broad_runtime = std::numeric_limits<uint64_t>::max();

  for (const auto& task : candidates) {
    remember_task_locked(task);
    if (!admission_allows(task.group_id)) {
      continue;
    }
    const double score = group_score(task.group_id);
    if (!best_task_id.has_value() || score < best_score ||
        (score == best_score && task.total_runtime_us < best_runtime)) {
      best_task_id = task.task_id;
      best_score = score;
      best_runtime = task.total_runtime_us;
    }
    // Tier 1: deeply underserved groups get priority.
    const double debt_key = compute_debt_key(task.group_id);
    if (debt_key < -5.0) {
      if (!tier1_task_id.has_value() || score < tier1_score ||
          (score == tier1_score && task.total_runtime_us < tier1_runtime)) {
        tier1_task_id = task.task_id;
        tier1_score = score;
        tier1_runtime = task.total_runtime_us;
      }
    }
    if (trace_enabled() && broad_debt_eligible) {
      const double broad_score = group_score_broad_debt(task.group_id);
      if (!shadow_broad_task_id.has_value() || broad_score < shadow_broad_score ||
          (broad_score == shadow_broad_score && task.total_runtime_us < shadow_broad_runtime)) {
        shadow_broad_task_id = task.task_id;
        shadow_broad_score = broad_score;
        shadow_broad_runtime = task.total_runtime_us;
      }
    }
  }
  if (!best_task_id.has_value() && total_running == 0) {
    for (const auto& task : candidates) {
      remember_task_locked(task);
      const double score = group_score(task.group_id);
      if (!best_task_id.has_value() || score < best_score ||
          (score == best_score && task.total_runtime_us < best_runtime)) {
        best_task_id = task.task_id;
        best_score = score;
        best_runtime = task.total_runtime_us;
      }
    }
  }

  // Promote tier-1 (deeply underserved) if reasonably close to best.
  if (tier1_task_id.has_value() && tier1_score < best_score + 2.0) {
    best_task_id = tier1_task_id;
  }

  // Trace: compare shadow broad-debt choice vs actual (no-debt) choice.
  if (trace_enabled()) {
    auto& t = trace();
    ++t.pick_total;
    if (window_debt_narrow || window_debt_broad) {
      ++t.pick_debt_changes;
    }
    if (broad_debt_eligible) {
      ++t.pick_debt_available;
      if (shadow_broad_task_id.has_value() && best_task_id.has_value() &&
          shadow_broad_task_id != best_task_id) {
        ++t.pick_debt_available_changes;
        // Categorize.
        if (max_active_weight == (4 * min_active_weight)) {
          ++t.cat_2g_4_1_no_block;
        } else if (max_active_weight >= (5 * min_active_weight)) {
          ++t.cat_2g_ge5_1_no_block;
        } else {
          ++t.cat_2g_other_no_block;
        }
      }
    }
  }

  return best_task_id;
}

schedlab::TickAction Scheduler::on_tick(const schedlab::TaskView& current, int worker_id,
                                        const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  account_runtime_locked(current);

  const uint64_t quantum_us = quantum_for_weight(system, current.weight);
  if (current.current_slice_runtime_us >= quantum_us) {
    return schedlab::TickAction::RequestResched;
  }

  if (current.current_slice_runtime_us < cfg_min_slice_before_preempt_us()) {
    return schedlab::TickAction::Continue;
  }

  if (worker_id < 0 || worker_id >= static_cast<int>(system.total_worker_count())) {
    return schedlab::TickAction::Continue;
  }

  const schedlab::QueueView& queue = system.queue(worker_id);
  if (queue.empty()) {
    return schedlab::TickAction::Continue;
  }

  const double current_key = fair_key_locked(current.group_id);
  double best_waiting_key = std::numeric_limits<double>::max();
  for (const auto& waiting : queue) {
    remember_task_locked(waiting);
    best_waiting_key = std::min(best_waiting_key, fair_key_locked(waiting.group_id));
  }

  const double tick_slack =
      std::max(cfg_tick_preempt_slack(), static_cast<double>(system.switch_cost()) * 0.75);
  if (best_waiting_key + tick_slack < current_key) {
    return schedlab::TickAction::RequestResched;
  }

  return schedlab::TickAction::Continue;
}

bool Scheduler::should_preempt(const schedlab::TaskView& waking, const schedlab::TaskView& current,
                               int, const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  remember_task_locked(waking);
  account_runtime_locked(current);

  if (waking.group_id == current.group_id) {
    return false;
  }
  if (current.current_slice_runtime_us < cfg_min_slice_before_preempt_us()) {
    return false;
  }

  std::unordered_map<uint64_t, uint32_t> running_by_group;
  std::unordered_map<uint64_t, uint32_t> runnable_by_group;
  std::unordered_set<uint64_t> active_groups;
  uint32_t total_running = 0;
  for (const auto& worker : system.worker_states()) {
    if (const auto running = system.running_task(worker.worker_id); running.has_value()) {
      remember_task_locked(*running);
      active_groups.insert(running->group_id);
      ++running_by_group[running->group_id];
      ++runnable_by_group[running->group_id];
      ++total_running;
    }
    const schedlab::QueueView& queue = system.queue(worker.worker_id);
    for (const auto& task : queue) {
      remember_task_locked(task);
      active_groups.insert(task.group_id);
      ++runnable_by_group[task.group_id];
    }
  }
  active_groups.insert(waking.group_id);

  refresh_active_groups_locked(system, active_groups);

  uint64_t total_active_weight = 0;
  for (const uint64_t group_id : active_groups) {
    const auto it = group_weight_.find(group_id);
    total_active_weight += normalized_weight(it == group_weight_.end() ? 1 : it->second);
  }

  const uint32_t total_workers = std::max<uint32_t>(1, system.total_worker_count());
  const double oversub_penalty =
      std::max(12.0, static_cast<double>(system.switch_cost()) * cfg_oversub_penalty_scale());
  const double parallelism_slack = std::max(0.0, cfg_parallelism_slack());

  auto group_score = [&](uint64_t group_id) {
    const double fair_key = fair_key_locked(group_id);
    if (total_active_weight == 0) {
      return fair_key;
    }

    const auto it = group_weight_.find(group_id);
    const uint64_t weight = normalized_weight(it == group_weight_.end() ? 1 : it->second);
    const double expected_parallelism =
        std::max(1.0, (static_cast<double>(total_workers) * static_cast<double>(weight)) /
                          static_cast<double>(total_active_weight));
    const double running_parallelism = static_cast<double>(running_by_group[group_id]);
    const double oversub =
        std::max(0.0, running_parallelism - expected_parallelism - parallelism_slack);
    const double weight_penalty_ratio =
        static_cast<double>(total_active_weight) / static_cast<double>(weight);
    return fair_key + (oversub * oversub_penalty * weight_penalty_ratio);
  };

  const double waking_score = group_score(waking.group_id);
  const double current_score = group_score(current.group_id);

  auto admission_enabled = [&]() {
    if (active_groups.size() != 2 || total_active_weight == 0) {
      return false;
    }
    uint64_t min_weight = std::numeric_limits<uint64_t>::max();
    uint64_t max_weight = 0;
    for (const uint64_t group_id : active_groups) {
      const auto blocked_it = group_has_blocked_.find(group_id);
      if (blocked_it != group_has_blocked_.end() && blocked_it->second) {
        return false;
      }
      const auto weight_it = group_weight_.find(group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      min_weight = std::min(min_weight, weight);
      max_weight = std::max(max_weight, weight);
    }
    return min_weight > 0 && max_weight == (4 * min_weight);
  }();

  auto desired_slots_for = [&](uint64_t group_id) {
    if (active_groups.size() <= 1 || total_active_weight == 0 || total_running == 0) {
      return static_cast<double>(total_workers);
    }

    double desired_total_running = static_cast<double>(total_workers);
    for (const uint64_t active_group_id : active_groups) {
      const auto weight_it = group_weight_.find(active_group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
      if (share <= 0.0) {
        continue;
      }
      desired_total_running = std::min(
          desired_total_running, static_cast<double>(runnable_by_group[active_group_id]) / share);
    }
    desired_total_running =
        std::clamp(desired_total_running, 1.0, static_cast<double>(total_workers));

    const auto weight_it = group_weight_.find(group_id);
    const uint64_t weight =
        normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
    const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
    return share * desired_total_running;
  };

  if (admission_enabled && total_running > 0) {
    const double current_slots = desired_slots_for(current.group_id);
    const double waking_slots = desired_slots_for(waking.group_id);
    const double slack = std::max(0.0, cfg_admission_slack());
    const bool current_over_slot =
        static_cast<double>(running_by_group[current.group_id]) > current_slots + slack;
    const bool waking_has_slot =
        static_cast<double>(running_by_group[waking.group_id] + 1) <= waking_slots + slack;
    if (current_over_slot && waking_has_slot) {
      return true;
    }
  }

  double wakeup_slack =
      std::max(cfg_wakeup_preempt_slack(), static_cast<double>(system.switch_cost()) * 0.5);
  const auto boost_it = group_boost_us_.find(waking.group_id);
  if (boost_it != group_boost_us_.end() && boost_it->second > 0) {
    const double scale = std::clamp(cfg_reactivation_wakeup_slack_scale(), 0.0, 1.0);
    wakeup_slack *= scale;
  }

  // Catch-up preemption boost from window debt (heavily damped).
  const double waking_debt = window_debt_key_locked(waking.group_id);
  const double current_debt = window_debt_key_locked(current.group_id);
  double catchup_boost = 0.0;
  if (waking_debt < 0.0) {
    catchup_boost += waking_debt * 0.05;  // negative debt → negative boost → easier preempt
  }
  if (current_debt > 0.0) {
    catchup_boost -= current_debt * 0.05; // overserved current → also easier preempt
  }

  if (waking_score + wakeup_slack + catchup_boost < current_score) {
    return true;
  }

  const uint64_t quantum_us = quantum_for_weight(system, current.weight);
  return current.current_slice_runtime_us >= quantum_us;
}

std::optional<schedlab::Scheduler::StealResult>
Scheduler::steal(int thief_worker_id, const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  if (system.total_worker_count() <= 1 || system.total_ready_count() == 0) {
    return std::nullopt;
  }

  std::unordered_map<uint64_t, uint32_t> running_by_group;
  std::unordered_map<uint64_t, uint32_t> runnable_by_group;
  std::unordered_set<uint64_t> active_groups;
  uint32_t total_running = 0;
  for (const auto& worker : system.worker_states()) {
    if (const auto running = system.running_task(worker.worker_id); running.has_value()) {
      remember_task_locked(*running);
      active_groups.insert(running->group_id);
      ++running_by_group[running->group_id];
      ++runnable_by_group[running->group_id];
      ++total_running;
    }
    const schedlab::QueueView& queue = system.queue(worker.worker_id);
    for (const auto& task : queue) {
      remember_task_locked(task);
      active_groups.insert(task.group_id);
      ++runnable_by_group[task.group_id];
    }
  }

  refresh_active_groups_locked(system, active_groups);

  uint64_t total_active_weight = 0;
  bool active_has_blocked = false;
  uint64_t min_active_weight = std::numeric_limits<uint64_t>::max();
  uint64_t max_active_weight = 0;
  for (const uint64_t group_id : active_groups) {
    const auto it = group_weight_.find(group_id);
    const uint64_t weight = normalized_weight(it == group_weight_.end() ? 1 : it->second);
    total_active_weight += weight;
    min_active_weight = std::min(min_active_weight, weight);
    max_active_weight = std::max(max_active_weight, weight);
    const auto blocked_it = group_has_blocked_.find(group_id);
    if (blocked_it != group_has_blocked_.end() && blocked_it->second) {
      active_has_blocked = true;
    }
  }
  const bool window_debt_narrow = active_groups.size() == 2 && !active_has_blocked &&
                                   min_active_weight > 0 &&
                                   max_active_weight == (4 * min_active_weight);

  const bool window_debt_broad =
      cfg_window_debt_broad_enabled() &&
      active_groups.size() >= 2 &&
      window_target_total_us_ >= cfg_window_debt_min_target_total_us() &&
      max_active_weight <= (cfg_window_debt_broad_max_ratio() * min_active_weight) &&
      !window_debt_narrow;

  const uint32_t total_workers = std::max<uint32_t>(1, system.total_worker_count());
  const double oversub_penalty =
      std::max(12.0, static_cast<double>(system.switch_cost()) * cfg_oversub_penalty_scale());
  const double parallelism_slack = std::max(0.0, cfg_parallelism_slack());

  auto compute_debt_key = [&](uint64_t group_id) -> double {
    if (window_debt_narrow) {
      return window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
    }
    if (window_debt_broad) {
      double debt = window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
      if (active_has_blocked) {
        if (debt >= -3.0) return 0.0;
        return debt;
      }
      return debt * 0.5;
    }
    return 0.0;
  };

  // Steal: ignore oversubscription. An idle worker should steal the most underserved group's task.
  auto group_score = [&](uint64_t group_id) {
    const double debt_key = compute_debt_key(group_id);
    return fair_key_locked(group_id) + debt_key;
  };

  // Shadow: compute what broad debt would choose when eligible but not active.
  const bool broad_debt_eligible = active_groups.size() == 2 && !active_has_blocked &&
                                    min_active_weight > 0 &&
                                    max_active_weight >= (4 * min_active_weight) &&
                                    !window_debt_narrow && !window_debt_broad;

  auto group_score_broad_debt = [&](uint64_t group_id) {
    const double debt_key =
        window_selection_debt_key_locked(group_id, min_active_weight, max_active_weight);
    return fair_key_locked(group_id) + debt_key;
  };

  auto admission_enabled = [&]() {
    if (active_groups.size() != 2 || total_active_weight == 0) {
      return false;
    }
    uint64_t min_weight = std::numeric_limits<uint64_t>::max();
    uint64_t max_weight = 0;
    for (const uint64_t group_id : active_groups) {
      const auto blocked_it = group_has_blocked_.find(group_id);
      if (blocked_it != group_has_blocked_.end() && blocked_it->second) {
        return false;
      }
      const auto weight_it = group_weight_.find(group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      min_weight = std::min(min_weight, weight);
      max_weight = std::max(max_weight, weight);
    }
    return min_weight > 0 && max_weight == (4 * min_weight);
  }();

  auto admission_allows = [&](uint64_t group_id) {
    if (!admission_enabled || total_running == 0) {
      return true;
    }
    double desired_total_running = static_cast<double>(total_workers);
    for (const uint64_t active_group_id : active_groups) {
      const auto weight_it = group_weight_.find(active_group_id);
      const uint64_t weight =
          normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
      const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
      if (share <= 0.0) continue;
      desired_total_running = std::min(
          desired_total_running, static_cast<double>(runnable_by_group[active_group_id]) / share);
    }
    desired_total_running =
        std::clamp(desired_total_running, 1.0, static_cast<double>(total_workers));
    const auto weight_it = group_weight_.find(group_id);
    const uint64_t weight =
        normalized_weight(weight_it == group_weight_.end() ? 1 : weight_it->second);
    const double share = static_cast<double>(weight) / static_cast<double>(total_active_weight);
    const double desired_slots = share * desired_total_running;
    return static_cast<double>(running_by_group[group_id] + 1) <=
           desired_slots + std::max(0.0, cfg_admission_slack());
  };

  std::optional<uint64_t> selected_task;
  double best_score = std::numeric_limits<double>::max();

  // Two-tier steal: first prefer deeply underserved groups (debt < -5.0).
  std::optional<uint64_t> tier1_task_id;
  double tier1_score = std::numeric_limits<double>::max();
  int tier1_victim = -1;

  // Shadow: track what broad debt would choose.
  std::optional<uint64_t> shadow_broad_task_id;
  double shadow_broad_score = std::numeric_limits<double>::max();
  uint64_t shadow_broad_runtime = std::numeric_limits<uint64_t>::max();

  int best_victim = -1;

  // Determine thief's NUMA node for topology-aware steal ordering.
  int thief_node = -1;
  for (const auto& ws : system.worker_states()) {
    if (ws.worker_id == thief_worker_id) {
      thief_node = ws.topology_node;
      break;
    }
  }

  // Two-pass steal: same NUMA node first, then cross-node fallback.
  auto scan_workers = [&](bool same_node_only) -> bool {
    bool found_any = false;
    for (const auto& worker : system.worker_states()) {
      if (worker.worker_id == thief_worker_id) continue;
      if (worker.local_queue_size == 0) continue;

      if (same_node_only && thief_node >= 0 && worker.topology_node != thief_node) continue;

      const uint64_t mig_cost = system.migration_cost(thief_worker_id, worker.worker_id);
      if (worker.local_queue_size <= 1 && mig_cost > (system.switch_cost() * 2)) continue;

      const schedlab::QueueView& queue = system.queue(worker.worker_id);
      for (const auto& task : queue) {
        remember_task_locked(task);
        if (!admission_allows(task.group_id)) continue;
        double score = group_score(task.group_id);
        if (!selected_task.has_value() || score < best_score) {
          selected_task = task.task_id;
          best_score = score;
          best_victim = worker.worker_id;
          found_any = true;
        }
        // Tier 1: deeply underserved groups get priority in steal.
        const double debt_key = compute_debt_key(task.group_id);
        if (debt_key < -5.0) {
          if (!tier1_task_id.has_value() || score < tier1_score) {
            tier1_task_id = task.task_id;
            tier1_score = score;
            tier1_victim = worker.worker_id;
          }
        }
        if (trace_enabled() && broad_debt_eligible) {
          const double broad_score = group_score_broad_debt(task.group_id);
          if (!shadow_broad_task_id.has_value() || broad_score < shadow_broad_score ||
              (broad_score == shadow_broad_score && task.total_runtime_us < shadow_broad_runtime)) {
            shadow_broad_task_id = task.task_id;
            shadow_broad_score = broad_score;
            shadow_broad_runtime = task.total_runtime_us;
          }
        }
      }
    }
    return found_any;
  };

  // First pass: same NUMA node only. If found, skip cross-node.
  if (!scan_workers(true)) {
    scan_workers(false);
  }

  // Promote tier-1 (deeply underserved) selection if available.
  if (tier1_task_id.has_value()) {
    selected_task = tier1_task_id;
    best_victim = tier1_victim;
  }

  // Trace: compare shadow broad-debt choice vs actual choice.
  if (trace_enabled()) {
    auto& t = trace();
    ++t.steal_total;
    if (window_debt_narrow || window_debt_broad) {
      ++t.steal_debt_changes;
    }
    if (broad_debt_eligible) {
      ++t.steal_debt_available;
      if (shadow_broad_task_id.has_value() && selected_task.has_value() &&
          shadow_broad_task_id != selected_task) {
        ++t.steal_debt_available_changes;
        if (max_active_weight == (4 * min_active_weight)) {
          ++t.cat_2g_4_1_no_block;
        } else if (max_active_weight >= (5 * min_active_weight)) {
          ++t.cat_2g_ge5_1_no_block;
        } else {
          ++t.cat_2g_other_no_block;
        }
      }
    }
  }

  if (!selected_task.has_value()) {
    return std::nullopt;
  }
  return StealResult{
      .victim_worker_id = best_victim,
      .task_id = selected_task,
  };
}

void Scheduler::on_task_preempted(const schedlab::TaskView& task, int worker_id) {
  (void)worker_id;
  std::lock_guard<std::mutex> lock(mu_);
  account_runtime_locked(task);
}

void Scheduler::on_task_blocked(const schedlab::TaskView& task, int worker_id) {
  (void)worker_id;
  std::lock_guard<std::mutex> lock(mu_);
  account_runtime_locked(task);
}

void Scheduler::on_task_exited(const schedlab::TaskView& task, int worker_id) {
  (void)worker_id;
  std::lock_guard<std::mutex> lock(mu_);
  account_runtime_locked(task);
  task_accounted_runtime_us_.erase(task.task_id);
  task_last_blocked_us_.erase(task.task_id);
}

} // namespace student
