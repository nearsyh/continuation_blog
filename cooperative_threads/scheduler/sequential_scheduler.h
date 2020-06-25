#ifndef SEQUENTIAL_SCHEDULER
#define SEQUENTIAL_SCHEDULER

#include <csetjmp>
#include <optional>
#include <queue>

#include "scheduler.h"

namespace nearsyh {
namespace scheduler {

class SequentialScheduler;

class SequentialTaskHolder : public TaskHolder {
 private:
  jmp_buf _jmp_target;
  friend class SequentialScheduler;
  void* _stack_bottom;
  void* _stack_top;
  int _stack_size;

 public:
  SequentialTaskHolder(void (*task)(Scheduler* scheduler)) : TaskHolder(task) {
    _stack_size = 16 * 1024;  // 16K
    _stack_bottom = malloc(_stack_size);
    _stack_top = static_cast<void*>(static_cast<uint8_t*>(_stack_bottom) + _stack_size);
  }

  virtual ~SequentialTaskHolder() { free(_stack_bottom); }

  virtual void run(Scheduler* scheduler);
};

enum SchedulerStatus { INIT = 0, SCHEDULE, EXIT };

class SequentialScheduler : public Scheduler {
 private:
  std::queue<SequentialTaskHolder*> _task_queue;
  jmp_buf _buf;
  SchedulerStatus _status;

 public:
  SequentialScheduler();

  ~SequentialScheduler();

  virtual void add_task(void (*task)(Scheduler* scheduler));

  virtual void yield();

  virtual void run();

 protected:
  virtual TaskHolder* get_current_task();

  virtual void set_current_task(TaskHolder* task_holder);

  virtual void exit_current_task();

 private:
  std::optional<SequentialTaskHolder*> choose_task();

  void schedule();

  void close();

  bool is_closed();
};

}  // namespace scheduler
}  // namespace nearsyh

#endif
