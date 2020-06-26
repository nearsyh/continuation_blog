#ifndef SCHEDULER
#define SCHEDULER

#include <memory>

namespace nearsyh {
namespace scheduler {

enum TaskStatus {
  CREATED = 0,
  RUN
};

enum SchedulerStatus { INIT = 0, SCHEDULE, EXIT };

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

 public:
  virtual ~Scheduler() {}

  virtual void add_task(void (*task)(Scheduler* scheduler)) = 0;

  virtual void yield() = 0;

  virtual void run() = 0;
};
}  // namespace scheduler
}  // namespace nearsyh

#endif
