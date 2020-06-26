#include <cstring>
#include <iostream>
#include <mutex>

#include "scheduler/parallel_scheduler.h"
#include "scheduler/scheduler.h"
#include "scheduler/sequential_scheduler.h"

using namespace nearsyh;

int main(int argc, char* argv[]) {
  scheduler::Scheduler* scheduler = nullptr;

  // Create different scheduler based on arguments
  if (argc == 2 && strcmp(argv[1], "parallel") == 0) {
    scheduler = new scheduler::ParallelScheduler();
  } else if (argc == 2 && strcmp(argv[1], "sequential") == 0) {
    scheduler = new scheduler::SequentialScheduler();
  } else {
    std::cout << "Usage: " << argv[0] << " [sequential|parallel]" << std::endl;
    return 1;
  }

  scheduler->add_task([](scheduler::Scheduler* scheduler) {
    for (int i = 0; i < 2; i++) {
      std::cout << "Task 1: " << i << std::endl;
      scheduler->yield();
    }
  });
  scheduler->add_task([](scheduler::Scheduler* scheduler) {
    for (int i = 0; i < 2; i++) {
      std::cout << "Task 2: " << i << std::endl;
      scheduler->yield();
    }
  });
  scheduler->run();
  std::cout << "Finish" << std::endl;
  delete scheduler;
  return 0;
}