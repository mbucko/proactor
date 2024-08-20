#ifndef THREAD_AFFINITY_H
#define THREAD_AFFINITY_H

#include <thread>

struct CoreInfo {
  int performanceCores = 0;
  int efficiencyCores = 0;
};

CoreInfo getCoreInfo();

void setThreadAffinity(std::thread& t, int coreId);

#endif  // THREAD_AFFINITY_H