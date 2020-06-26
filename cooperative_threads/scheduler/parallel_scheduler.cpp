#include "parallel_scheduler.h"

#include <iostream>
#include <thread>

namespace nearsyh {
namespace scheduler {

static thread_local int thread_id = -1;

ParallelScheduler::ParallelScheduler() {
  _task_queue = std::queue<ParallelTaskHolder*>{};
  _current_tasks =
      std::vector<ParallelTaskHolder*>(std::thread::hardware_concurrency());
  for (int i = 0; i < std::thread::hardware_concurrency(); i++) {
    _current_tasks[i] = nullptr;
  }
}

ParallelScheduler::~ParallelScheduler() {}

bool ParallelScheduler::is_closed() { return _status == SchedulerStatus::EXIT; }

void ParallelScheduler::close() { _status = SchedulerStatus::EXIT; }

void ParallelScheduler::add_task(void (*task)(Scheduler* scheduler)) {
  std::lock_guard<std::mutex> guard{this->_mutex};
  if (is_closed()) {
    throw "This scheduler has finished its tasks.";
  }
  _task_queue.push(new ParallelTaskHolder(task));
}

std::optional<ParallelTaskHolder*> ParallelScheduler::choose_task() {
  if (this->_task_queue.empty()) {
    return std::nullopt;
  }
  auto ret = std::optional<ParallelTaskHolder*>(_task_queue.front());
  _task_queue.pop();
  return ret;
}

void ParallelScheduler::schedule() {
  std::optional<ParallelTaskHolder*> next_task_opt = std::nullopt;
  {
    std::lock_guard<std::mutex> guard{this->_mutex};
    next_task_opt = choose_task();
    if (!next_task_opt.has_value()) {
      return;
    }
  }

  auto* next_task = next_task_opt.value();
  set_current_task(next_task);

  if (next_task->_status == TaskStatus::CREATED) {
    next_task->_status = TaskStatus::RUN;
    // Load Stack
    auto* top = next_task->_stack_top;
    asm volatile("mov %[rs], %%rsp \n" : [ rs ] "+r"(top)::);

    get_current_task()->run(this);
    this->exit_current_task();
  } else {
    longjmp(next_task->_jmp_target, 1);
  }
}

TaskHolder* ParallelScheduler::get_current_task() {
  return _current_tasks[thread_id];
}

void ParallelScheduler::set_current_task(TaskHolder* task_holder) {
  _current_tasks[thread_id] = static_cast<ParallelTaskHolder*>(task_holder);
}

void ParallelScheduler::exit_current_task() {
  delete get_current_task();
  set_current_task(nullptr);
  longjmp(_buf, SchedulerStatus::EXIT);
}

void ParallelScheduler::yield() {
  auto* task_holder = static_cast<ParallelTaskHolder*>(get_current_task());
  if (setjmp(task_holder->_jmp_target)) {
    return;
  } else {
    {
      std::lock_guard<std::mutex> guard(_mutex);
      _task_queue.push(task_holder);
    }
    longjmp(_buf, SchedulerStatus::SCHEDULE);
  }
}

void ParallelScheduler::run() {
  auto threads = std::vector<std::thread>{};
  for (int i = 0; i < _current_tasks.size(); i++) {
    threads.push_back(std::thread([&]() {
      thread_id = i;
      switch (setjmp(_buf)) {
        case SchedulerStatus::EXIT:
        case SchedulerStatus::INIT:
        case SchedulerStatus::SCHEDULE:
          schedule();
          return;
        default:
          break;
      }
    }));
  }
  for (auto& thread : threads) {
    thread.join();
  }
}

}  // namespace scheduler
}  // namespace nearsyh