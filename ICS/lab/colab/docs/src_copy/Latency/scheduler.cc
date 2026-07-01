#include "scheduler.h"

#include <algorithm>
#include <limits>

namespace student {

namespace {

constexpr uint64_t kForegroundWeight = 1;
constexpr uint64_t kMinSliceBeforePreemptUs = 1;
constexpr uint64_t kPureInteractiveQuantumUs = 20;
constexpr uint64_t kForegroundQuantumUs = 24;
constexpr uint64_t kBackgroundQuantumUs = 1;
constexpr uint64_t kBackgroundOnlyQuantumUs = 64;

bool has_blocked_before(const schedlab::TaskView& task) noexcept {
  return task.voluntary_block_count > 0 ||
         task.last_stop_reason == schedlab::StopReason::Blocked;
}

bool is_foreground(const schedlab::TaskView& task) noexcept {
  return task.weight <= kForegroundWeight || has_blocked_before(task);
}

int priority_class(const schedlab::TaskView& task) noexcept {
  if (has_blocked_before(task)) {
    return 0;
  }
  if (task.weight <= kForegroundWeight) {
    return 1;
  }
  return 2;
}

int interactive_priority_class(const schedlab::TaskView& task) noexcept {
  return has_blocked_before(task) ? 0 : 1;
}

struct TaskKey {
  int cls = 0;
  uint64_t runtime = 0;
  uint64_t weight = 0;
  std::size_t queue_index = 0;
};

TaskKey key_for(const schedlab::TaskView& task, std::size_t queue_index = 0,
                bool mixed_latency_mode = true) noexcept {
  return TaskKey{
      .cls = mixed_latency_mode ? priority_class(task) : interactive_priority_class(task),
      .runtime = mixed_latency_mode ? task.total_runtime_us : 0,
      .weight = task.weight,
      .queue_index = queue_index,
  };
}

bool better_key(const TaskKey& lhs, const TaskKey& rhs) noexcept {
  if (lhs.cls != rhs.cls) return lhs.cls < rhs.cls;
  if (lhs.runtime != rhs.runtime) return lhs.runtime < rhs.runtime;
  if (lhs.weight != rhs.weight) return lhs.weight < rhs.weight;
  return lhs.queue_index < rhs.queue_index;
}

bool better_task(const schedlab::TaskView& lhs, const schedlab::TaskView& rhs) noexcept {
  return better_key(key_for(lhs), key_for(rhs));
}

bool better_task(const schedlab::TaskView& lhs, const schedlab::TaskView& rhs,
                 bool mixed_latency_mode) noexcept {
  return better_key(key_for(lhs, 0, mixed_latency_mode),
                    key_for(rhs, 0, mixed_latency_mode));
}

bool meaningfully_better(const schedlab::TaskView& lhs,
                         const schedlab::TaskView& rhs,
                         bool mixed_latency_mode = true) noexcept {
  const int lhs_class =
      mixed_latency_mode ? priority_class(lhs) : interactive_priority_class(lhs);
  const int rhs_class =
      mixed_latency_mode ? priority_class(rhs) : interactive_priority_class(rhs);
  if (lhs_class != rhs_class) {
    return lhs_class < rhs_class;
  }
  return mixed_latency_mode && lhs.weight < rhs.weight;
}

int safe_worker(int worker_id, uint32_t n_workers) noexcept {
  if (n_workers == 0) return 0;
  if (worker_id < 0 || worker_id >= static_cast<int>(n_workers)) return -1;
  return worker_id;
}

int worker_node(const schedlab::SystemView& system, int worker_id) {
  for (const auto& worker : system.worker_states()) {
    if (worker.worker_id == worker_id) {
      return worker.topology_node;
    }
  }
  return -1;
}

bool worker_is_idle(const schedlab::SystemView& system, int worker_id) {
  for (const auto& worker : system.worker_states()) {
    if (worker.worker_id == worker_id) {
      return worker.is_idle;
    }
  }
  return false;
}

uint64_t placement_load(const schedlab::TaskView& task,
                        const schedlab::SystemView::WorkerState& worker,
                        const schedlab::SystemView& system, int preferred_worker) {
  uint64_t load = static_cast<uint64_t>(worker.local_queue_size) * 16;
  if (!worker.is_idle) {
    load += 12;
  }

  if (worker.running_task_id.has_value()) {
    const auto running = system.task(*worker.running_task_id);
    if (running.has_value()) {
      if (is_foreground(task) && !is_foreground(*running)) {
        load = (load >= 8) ? load - 8 : 0;
      } else if (!is_foreground(task) && is_foreground(*running)) {
        load += 16;
      }
    }
  }

  if (worker.worker_id == preferred_worker) {
    load = (load >= 2) ? load - 2 : 0;
  }
  return load;
}

uint64_t foreground_waiting_load(int worker_id, const schedlab::SystemView& system) {
  const schedlab::QueueView& queue = system.queue(worker_id);
  uint64_t load = 0;
  for (std::size_t i = 0; i < queue.size(); ++i) {
    const auto& queued = queue[i];
    if (!is_foreground(queued)) {
      continue;
    }
    load += has_blocked_before(queued) ? 2 : 3;
  }
  return load;
}

} // namespace

void Scheduler::observe_task(const schedlab::TaskView& task) noexcept {
  if (task.weight > kForegroundWeight) {
    seen_background_weight_.store(true, std::memory_order_relaxed);
  }
}

bool Scheduler::mixed_latency_mode() const noexcept {
  return seen_background_weight_.load(std::memory_order_relaxed);
}

void Scheduler::init(const schedlab::SystemView& system) {
  std::lock_guard<std::mutex> lock(mu_);
  seen_background_weight_.store(false, std::memory_order_relaxed);
  next_spawn_worker_ = 0;
  next_spawn_node_ = 0;
  const uint32_t n_nodes = system.node_count();
  next_worker_in_node_.assign(n_nodes > 0 ? n_nodes : 1, 0);
}

int Scheduler::select_worker(const schedlab::TaskView& task, schedlab::ReadyContext ctx,
                             const schedlab::SystemView& system) {
  observe_task(task);
  std::lock_guard<std::mutex> lock(mu_);
  const uint32_t n_workers = system.total_worker_count();
  if (n_workers == 0) return 0;

  const int source = safe_worker(ctx.source_worker_id, n_workers);
  const int previous = safe_worker(ctx.previous_worker_id, n_workers);
  const int preferred = source >= 0 ? source : previous;
  const bool mixed_mode = mixed_latency_mode();

  if (ctx.reason == schedlab::ReadyReason::Wakeup && !mixed_mode &&
      (source < 0 || (task.last_worker_id == source && task.total_runtime_us > 0))) {
    if (source >= 0) return source;
    if (previous >= 0) return previous;
    return 0;
  }

  if (ctx.reason == schedlab::ReadyReason::Spawn && (!mixed_mode || !is_foreground(task))) {
    const uint32_t n_nodes = system.node_count();
    if (n_nodes > 1) {
      const int node = next_spawn_node_ % static_cast<int>(n_nodes);
      next_spawn_node_ = (node + 1) % static_cast<int>(n_nodes);
      auto workers = system.workers_in_node(node);
      if (!workers.empty()) {
        int& index = next_worker_in_node_[static_cast<std::size_t>(node)];
        const int worker = workers[static_cast<std::size_t>(index) % workers.size()];
        index = (index + 1) % static_cast<int>(workers.size());
        return worker;
      }
    }

    const int worker = next_spawn_worker_ % static_cast<int>(n_workers);
    next_spawn_worker_ = (worker + 1) % static_cast<int>(n_workers);
    return worker;
  }

  int best_worker = 0;
  uint64_t best_load = std::numeric_limits<uint64_t>::max();
  int best_migration_node_delta = std::numeric_limits<int>::max();
  int best_preferred_delta = std::numeric_limits<int>::max();
  const int preferred_node = preferred >= 0 ? worker_node(system, preferred) : -1;
  const bool foreground_task = is_foreground(task);

  for (const auto& worker : system.worker_states()) {
    uint64_t load = placement_load(task, worker, system, preferred);
    if (foreground_task) {
      load = foreground_waiting_load(worker.worker_id, system);
      if (!worker.is_idle) {
        const auto running = system.running_task(worker.worker_id);
        if (running.has_value() && is_foreground(*running)) {
          load += has_blocked_before(*running) ? 2 : 3;
        }
      }
      if (worker.worker_id == preferred) {
        load = (load >= 1) ? load - 1 : 0;
      }
    }

    int node_delta = 0;
    if (preferred_node >= 0 && worker.topology_node != preferred_node) {
      node_delta = 1;
    }
    const int preferred_delta =
        foreground_task && preferred >= 0 && worker.worker_id != preferred ? 1 : 0;

    if (load < best_load || (load == best_load && node_delta < best_migration_node_delta) ||
        (load == best_load && node_delta == best_migration_node_delta &&
         preferred_delta < best_preferred_delta) ||
        (load == best_load && node_delta == best_migration_node_delta &&
         preferred_delta == best_preferred_delta &&
         worker.worker_id < best_worker)) {
      best_load = load;
      best_worker = worker.worker_id;
      best_migration_node_delta = node_delta;
      best_preferred_delta = preferred_delta;
    }
  }

  if (preferred >= 0 && foreground_task && worker_is_idle(system, preferred)) {
    const uint64_t preferred_load = foreground_waiting_load(preferred, system);
    if (preferred_load <= best_load) {
      return preferred;
    }
  }

  return best_worker;
}

std::optional<uint64_t> Scheduler::pick_next(int, schedlab::QueueView candidates,
                                             const schedlab::SystemView&) {
  if (candidates.empty()) return std::nullopt;

  for (std::size_t i = 0; i < candidates.size(); ++i) {
    observe_task(candidates[i]);
  }
  const bool mixed_mode = mixed_latency_mode();
  if (!mixed_mode) {
    return candidates.front().task_id;
  }

  std::optional<uint64_t> best_task_id;
  TaskKey best_key{};
  for (std::size_t i = 0; i < candidates.size(); ++i) {
    const auto& task = candidates[i];
    const TaskKey key = key_for(task, i, mixed_mode);
    if (!best_task_id.has_value() || better_key(key, best_key)) {
      best_task_id = task.task_id;
      best_key = key;
    }
  }
  return best_task_id;
}

schedlab::TickAction Scheduler::on_tick(const schedlab::TaskView& current, int worker_id,
                                        const schedlab::SystemView& system) {
  observe_task(current);
  if (worker_id < 0 || worker_id >= static_cast<int>(system.total_worker_count())) {
    return schedlab::TickAction::Continue;
  }

  const schedlab::QueueView& queue = system.queue(worker_id);
  if (queue.empty()) {
    return schedlab::TickAction::Continue;
  }

  const bool mixed_mode = mixed_latency_mode();
  if (!mixed_mode) {
    return current.current_slice_runtime_us >= kPureInteractiveQuantumUs
               ? schedlab::TickAction::RequestResched
               : schedlab::TickAction::Continue;
  }

  const schedlab::TaskView* best_waiting = nullptr;
  for (std::size_t i = 0; i < queue.size(); ++i) {
    const auto& task = queue[i];
    observe_task(task);
    if (best_waiting == nullptr || better_task(task, *best_waiting, mixed_mode)) {
      best_waiting = &task;
    }
  }

  if (best_waiting != nullptr && current.current_slice_runtime_us >= kMinSliceBeforePreemptUs &&
      meaningfully_better(*best_waiting, current, mixed_mode)) {
    return schedlab::TickAction::RequestResched;
  }

  if (is_foreground(current)) {
    if (best_waiting != nullptr && is_foreground(*best_waiting) &&
        current.current_slice_runtime_us >= kForegroundQuantumUs) {
      return schedlab::TickAction::RequestResched;
    }
    return schedlab::TickAction::Continue;
  }

  if (best_waiting != nullptr && is_foreground(*best_waiting) &&
      current.current_slice_runtime_us >= kBackgroundQuantumUs) {
    return schedlab::TickAction::RequestResched;
  }
  if (current.current_slice_runtime_us >= kBackgroundOnlyQuantumUs) {
    return schedlab::TickAction::RequestResched;
  }
  return schedlab::TickAction::Continue;
}

bool Scheduler::should_preempt(const schedlab::TaskView& waking,
                               const schedlab::TaskView& current, int,
                               const schedlab::SystemView&) {
  observe_task(waking);
  observe_task(current);
  if (current.current_slice_runtime_us < kMinSliceBeforePreemptUs) {
    return false;
  }
  const bool mixed_mode = mixed_latency_mode();
  if (!mixed_mode) {
    return has_blocked_before(waking) &&
           (waking.total_runtime_us == 0 || !has_blocked_before(current));
  }
  return meaningfully_better(waking, current, mixed_mode);
}

std::optional<schedlab::Scheduler::StealResult>
Scheduler::steal(int thief_worker_id, const schedlab::SystemView& system) {
  const uint32_t n_workers = system.total_worker_count();
  if (n_workers <= 1 || system.total_ready_count() == 0) {
    return std::nullopt;
  }

  const bool mixed_mode = mixed_latency_mode();
  if (!mixed_mode) {
    return std::nullopt;
  }

  int best_victim = -1;
  std::optional<uint64_t> best_task_id;
  TaskKey best_key{};
  uint32_t best_victim_queue = 0;
  uint64_t best_migration_cost = std::numeric_limits<uint64_t>::max();

  for (const auto& worker : system.worker_states()) {
    if (worker.worker_id == thief_worker_id || worker.local_queue_size <= 1) {
      continue;
    }
    const schedlab::QueueView& queue = system.queue(worker.worker_id);
    for (std::size_t i = 0; i < queue.size(); ++i) {
      const auto& task = queue[i];
      observe_task(task);
      TaskKey key = key_for(task, i, mixed_mode);
      const uint64_t migration_cost = system.migration_cost(worker.worker_id, thief_worker_id);
      if (!best_task_id.has_value() || better_key(key, best_key) ||
          (!better_key(best_key, key) &&
           (worker.local_queue_size > best_victim_queue ||
            (worker.local_queue_size == best_victim_queue &&
             migration_cost < best_migration_cost)))) {
        best_task_id = task.task_id;
        best_key = key;
        best_victim = worker.worker_id;
        best_victim_queue = worker.local_queue_size;
        best_migration_cost = migration_cost;
      }
    }
  }

  if (!best_task_id.has_value() || best_victim < 0) {
    return std::nullopt;
  }

  const auto task = system.task(*best_task_id);
  if (task.has_value() && !is_foreground(*task) && best_victim_queue <= 1 &&
      best_migration_cost > system.switch_cost() * 2) {
    return std::nullopt;
  }

  return StealResult{
      .victim_worker_id = best_victim,
      .task_id = best_task_id,
  };
}

} // namespace student
