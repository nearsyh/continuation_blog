#ifndef SCHEDULER
#define SCHEDULER

#include <memory>

namespace nearsyh {
namespace scheduler {

class Scheduler;

class TaskHolder {
 private:
  void (*_task)(Scheduler* scheduler);

 public:
  TaskHolder(void (*task)(Scheduler* scheduler))
      : _task(task){

        };

  virtual ~TaskHolder() {}

  virtual void run(Scheduler* scheduler) { _task(scheduler); }
};

class Scheduler {
 private:
  friend class TaskHolder;

 protected:
  virtual TaskHolder* get_current_task() = 0;

  virtual void set_current_task(TaskHolder* task_holder) = 0;

  virtual void exit_current_task() = 0;

 public:
  virtual ~Scheduler() {}

  virtual void add_task(void (*task)(Scheduler* scheduler)) = 0;

  virtual void yield() = 0;

  virtual void run() = 0;
};
}  // namespace scheduler
}  // namespace nearsyh

#endif
