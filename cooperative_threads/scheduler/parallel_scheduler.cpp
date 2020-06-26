#include "parallel_scheduler.h"

#include <iostream>
#include <thread>

namespace nearsyh {
namespace scheduler {

static thread_local int thread_id = -1;
static thread_local jmp_buf thread_local_buf;

ParallelScheduler::ParallelScheduler() {
  _task_queue = std::queue<ParallelTaskHolder*>{};
  _current_tasks = std::vector<ParallelTaskHolder*>();
  int concurrency = std::thread::hardware_concurrency();
  concurrency = 2;
  _bufs = new jmp_buf[concurrency];
  for (int i = 0; i < concurrency; i++) {
    _current_tasks.push_back(nullptr);
  }
}

ParallelScheduler::~ParallelScheduler() {
  delete _bufs;
}

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
    std::cout << "Thread " << thread_id << " pick a task from "
              << _task_queue.size() << " tasks." << std::endl;
    next_task_opt = choose_task();
    if (!next_task_opt.has_value()) {
      std::cout << "Thread " << thread_id << " returns" << std::endl;
      return;
    }
  }

  auto* next_task = next_task_opt.value();
  set_current_task(next_task);
  {
    std::lock_guard<std::mutex> guard{this->_mutex};
    std::cout << "Thread " << thread_id << ": " << next_task << std::endl;
  }

  if (next_task->_status == TaskStatus::CREATED) {
    next_task->_status = TaskStatus::RUN;
    // Load Stack
    auto* top = next_task->_stack_top;
    asm volatile("mov %[rs], %%rsp \n" : [ rs ] "+r"(top)::);

    get_current_task()->run(this);
    {
      std::lock_guard<std::mutex> guard{this->_mutex};
      std::cout << "Thread " << thread_id << " exiting " << std::endl;
      std::cout << "Delete " << get_current_task() << std::endl;
    }
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
  longjmp(thread_local_buf, SchedulerStatus::EXIT);
}

void ParallelScheduler::yield() {
  {
    std::lock_guard<std::mutex> guard(_mutex);
    std::cout << "Thread " << thread_id << " yield from task " << get_current_task() << std::endl; 
  }
  auto* task_holder = static_cast<ParallelTaskHolder*>(get_current_task());
  if (setjmp(task_holder->_jmp_target)) {
    return;
  } else {
    {
      std::lock_guard<std::mutex> guard(_mutex);
      _task_queue.push(task_holder);
    }
    longjmp(thread_local_buf, SchedulerStatus::SCHEDULE);
  }
}

void ParallelScheduler::run() {
  auto threads = std::vector<std::thread>{};
  for (int i = 0; i < _current_tasks.size(); i++) {
    threads.push_back(std::thread([&]() {
      thread_id = i;
      {
        std::lock_guard<std::mutex> guard(_mutex);
        std::cout << "Jmp buf " << thread_local_buf << std::endl;
      }
      switch (setjmp(thread_local_buf)) {
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
  int c = 0;
  for (auto& thread : threads) {
    thread.join();
    std::cout << "Join " << c << std::endl;
    c += 1;
  }
}

}  // namespace scheduler
}  // namespace nearsyh