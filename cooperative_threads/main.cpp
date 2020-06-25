#include <iostream>

#include "scheduler/scheduler.h"
#include "scheduler/sequential_scheduler.h"

using namespace nearsyh;

int main() {
  scheduler::SequentialScheduler scheduler{};
  scheduler.add_task([](scheduler::Scheduler *scheduler) {
    for (int i = 0; i < 10; i++) {
      std::cout << "Task 1" << std::endl;
      scheduler->yield();
    }
  });
  scheduler.add_task([](scheduler::Scheduler *scheduler) {
    for (int i = 0; i < 20; i++) {
      std::cout << "Task 2" << std::endl;
      scheduler->yield();
    }
  });
  scheduler.run();
  std::cout << "Finish" << std::endl;
  return 0;
}