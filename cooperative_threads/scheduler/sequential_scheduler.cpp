#include "sequential_scheduler.h"

namespace nearsyh {
namespace scheduler {

template <typename TO, typename FROM>
std::unique_ptr<TO> static_unique_pointer_cast(std::unique_ptr<FROM>&& old) {
  return std::unique_ptr<TO>{static_cast<TO*>(old.release())};
}

SequentialScheduler::SequentialScheduler() {
  _task_queue = std::queue<SequentialTaskHolder*>{};
}

SequentialScheduler::~SequentialScheduler() {}

bool SequentialScheduler::is_closed() {
  return _status == SchedulerStatus::EXIT;
}

void SequentialScheduler::close() { _status = SchedulerStatus::EXIT; }

void SequentialScheduler::add_task(void (*task)(Scheduler* scheduler)) {
  if (is_closed()) {
    throw "This scheduler has finished its tasks.";
  }
  _task_queue.push(new SequentialTaskHolder(task));
}

std::optional<SequentialTaskHolder*> SequentialScheduler::choose_task() {
  if (this->_task_queue.empty()) {
    return std::nullopt;
  }
  auto ret = std::optional<SequentialTaskHolder*>(_task_queue.front());
  _task_queue.pop();
  return ret;
}

void SequentialScheduler::schedule() {
  auto next_task_opt = choose_task();
  if (!next_task_opt.has_value()) {
    return;
  }

  // Load Stack
  auto* top = next_task_opt.value()->_stack_top;
  asm volatile("mov %[rs], %%rsp \n" : [ rs ] "+r"(top)::);

  set_current_task(next_task_opt.value());
  get_current_task()->run(this);
  this->exit_current_task();
}

void SequentialScheduler::exit_current_task() {
  delete get_current_task();
  set_current_task(nullptr);
  longjmp(_buf, SchedulerStatus::EXIT);
}

void SequentialScheduler::yield() {
  auto* task_holder = static_cast<SequentialTaskHolder*>(get_current_task());
  if (setjmp(task_holder->_jmp_target)) {
    return;
  } else {
    _task_queue.push(task_holder);
    longjmp(_buf, SchedulerStatus::SCHEDULE);
  }
}

void SequentialScheduler::run() {
  switch (setjmp(_buf)) {
    case SchedulerStatus::EXIT:
    case SchedulerStatus::INIT:
    case SchedulerStatus::SCHEDULE:
      schedule();
      return;
    default:
      break;
  }
}

}  // namespace scheduler
}  // namespace nearsyh