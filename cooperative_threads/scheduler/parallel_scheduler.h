#ifndef PARALLEL_SCHEDULER
#define PARALLEL_SCHEDULER

#include <csetjmp>
#include <optional>
#include <queue>
#include <mutex>
#include <vector>

#include "scheduler.h"

namespace nearsyh {
namespace scheduler {

class ParallelScheduler;

class ParallelTaskHolder : public TaskHolder {
 private:
  jmp_buf _jmp_target;
  friend class ParallelScheduler;
  void* _stack_bottom;
  void* _stack_top;
  int _stack_size;
  TaskStatus _status;

 public:
  ParallelTaskHolder(void (*task)(Scheduler* scheduler)) : TaskHolder(task) {
    _stack_size = 16 * 1024;  // 16K
    _stack_bottom = malloc(_stack_size);
    _stack_top = static_cast<void*>(static_cast<uint8_t*>(_stack_bottom) + _stack_size);
  }

  virtual ~ParallelTaskHolder() { free(_stack_bottom); }

  virtual void run(Scheduler* scheduler) {
    TaskHolder::run(scheduler);
  }
};

class ParallelScheduler : public Scheduler {
 private:
  mutable std::mutex _mutex;
  std::vector<ParallelTaskHolder*> _current_tasks;
  std::queue<ParallelTaskHolder*> _task_queue;
  jmp_buf _buf;
  SchedulerStatus _status;

 public:
  ParallelScheduler();

  ~ParallelScheduler();

  virtual void add_task(void (*task)(Scheduler* scheduler));

  virtual void yield();

  virtual void run();

 protected:
  virtual TaskHolder* get_current_task();

  virtual void set_current_task(TaskHolder* task_holder);

  virtual void exit_current_task();

 private:
  std::optional<ParallelTaskHolder*> choose_task();

  void schedule();

  void close();

  bool is_closed();
};

}  // namespace scheduler
}  // namespace nearsyh

#endif
