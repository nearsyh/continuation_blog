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

  virtual ~TaskHolder();

  virtual void run(Scheduler* scheduler) { _task(scheduler); }
};

class Scheduler {
 private:
  TaskHolder* _current_task;
  friend class TaskHolder;

 protected:
  virtual TaskHolder* get_current_task();

  virtual void set_current_task(TaskHolder* task_holder);

  virtual void exit_current_task();

 public:
  virtual ~Scheduler();

  virtual void add_task(void (*task)(Scheduler* scheduler));

  virtual void yield();

  virtual void run();
};
}  // namespace scheduler
}  // namespace nearsyh

#endif
