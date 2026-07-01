#pragma once

#include <cstdint>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "schedlab/scheduler.hpp"

namespace student {

class Scheduler final : public schedlab::Scheduler {
public:
  void init(const schedlab::SystemView& system) override;

  int select_worker(const schedlab::TaskView& task, schedlab::ReadyContext ctx,
                    const schedlab::SystemView& system) override;

  std::optional<uint64_t> pick_next(int worker_id, schedlab::QueueView candidates,
                                    const schedlab::SystemView& system) override;

  schedlab::TickAction on_tick(const schedlab::TaskView& current, int worker_id,
                               const schedlab::SystemView& system) override;

  bool should_preempt(const schedlab::TaskView& waking, const schedlab::TaskView& current,
                      int worker_id, const schedlab::SystemView& system) override;

  std::optional<StealResult> steal(int thief_worker_id,
                                   const schedlab::SystemView& system) override;

  void on_task_preempted(const schedlab::TaskView& task, int worker_id) override;
  void on_task_blocked(const schedlab::TaskView& task, int worker_id) override;
  void on_task_exited(const schedlab::TaskView& task, int worker_id) override;

private:
  uint64_t normalized_weight(uint64_t weight) const;
  int choose_least_loaded_worker(const schedlab::SystemView& system, int preferred_worker);

  void remember_task_locked(const schedlab::TaskView& task);
  void account_runtime_locked(const schedlab::TaskView& task);
  void account_window_runtime_locked(uint64_t group_id, uint64_t delta_us);
  double fair_key_locked(uint64_t group_id) const;
  double window_debt_key_locked(uint64_t group_id) const;
  double window_selection_debt_key_locked(uint64_t group_id, uint64_t min_active_weight,
                                          uint64_t max_active_weight) const;
  void refresh_active_groups_locked(const schedlab::SystemView& system,
                                    const std::unordered_set<uint64_t>& active_groups);

  uint64_t quantum_for_weight(const schedlab::SystemView& system, uint64_t weight) const;

  mutable std::mutex mu_;
  int next_spawn_worker_ = 0;
  uint64_t active_epoch_ = 0;

  std::unordered_map<uint64_t, uint64_t> task_accounted_runtime_us_;
  std::unordered_map<uint64_t, uint64_t> task_last_blocked_us_;
  std::unordered_map<uint64_t, uint64_t> group_weight_;
  std::unordered_map<uint64_t, uint64_t> group_service_us_;
  std::unordered_map<uint64_t, uint64_t> group_boost_us_;
  std::unordered_map<uint64_t, uint64_t> group_recent_blocked_us_;
  std::unordered_map<uint64_t, bool> group_has_blocked_;
  std::unordered_map<uint64_t, uint64_t> group_last_active_epoch_;
  std::unordered_map<uint64_t, double> group_window_actual_us_;
  std::unordered_map<uint64_t, double> group_window_target_us_;
  std::unordered_set<uint64_t> last_active_groups_;
  uint64_t last_active_total_weight_ = 0;
  double window_target_total_us_ = 0.0;
};

} // namespace student
