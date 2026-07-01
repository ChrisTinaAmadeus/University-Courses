#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

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

private:
  void observe_task(const schedlab::TaskView& task) noexcept;
  bool mixed_latency_mode() const noexcept;

  mutable std::mutex mu_;
  int next_spawn_worker_ = 0;
  int next_spawn_node_ = 0;
  std::vector<int> next_worker_in_node_;
  std::atomic<bool> seen_background_weight_{false};
};

} // namespace student
