#ifndef THREADAFFINITY_H
#define THREADAFFINITY_H

#include <thread>

namespace mbucko {

struct CoreInfo {
  int performanceCores = 0;
  int efficiencyCores = 0;
};

CoreInfo getCoreInfo();

void setThreadAffinity(std::thread& t, int coreId);

}  // namespace mbucko

#endif  // THREADAFFINITY_H