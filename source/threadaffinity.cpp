#include "threadaffinity.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#ifdef __APPLE__
#include <mach/mach_error.h>
#include <mach/thread_act.h>
#include <mach/thread_policy.h>
#include <sys/sysctl.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

CoreInfo getCoreInfo() {
  CoreInfo info;
#ifdef __APPLE__
  int count;
  size_t size = sizeof(count);
  if (sysctlbyname("hw.perflevel0.logicalcpu", &count, &size, NULL, 0) == 0) {
    info.performanceCores = count;
  }
  if (sysctlbyname("hw.perflevel1.logicalcpu", &count, &size, NULL, 0) == 0) {
    info.efficiencyCores = count;
  }
  if (info.performanceCores == 0 && info.efficiencyCores == 0) {
    info.performanceCores = std::thread::hardware_concurrency();
  }
#else
  info.performanceCores = std::thread::hardware_concurrency();
#endif
  return info;
}

void setThreadAffinity(std::thread& t, int coreId) {
#ifdef __APPLE__
  thread_affinity_policy_data_t policy = {coreId};
  thread_port_t mach_thread = pthread_mach_thread_np(t.native_handle());
  kern_return_t result = thread_policy_set(mach_thread, THREAD_AFFINITY_POLICY,
                                           (thread_policy_t)&policy, 1);
  if (result == KERN_NOT_SUPPORTED) {
    static bool notSupportedErrorPrinted = false;
    if (!notSupportedErrorPrinted) {
      std::cerr
          << "Warning: Thread affinity is not supported on this Mac system."
          << std::endl;
      notSupportedErrorPrinted = true;
    }
  } else if (result != KERN_SUCCESS) {
    std::cerr << "Error setting thread affinity on macOS: "
              << mach_error_string(result) << std::endl;
  }
#elif defined(_WIN32)
  DWORD_PTR mask = (1ULL << coreId);
  DWORD_PTR result = SetThreadAffinityMask(t.native_handle(), mask);
  if (result == 0) {
    DWORD error = GetLastError();
    char errorMsg[256];
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   errorMsg, sizeof(errorMsg), NULL);
    std::cerr << "Error setting thread affinity on Windows: " << errorMsg
              << std::endl;
  }
#else
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(coreId, &cpuset);
  int result =
      pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
  if (result != 0) {
    std::cerr << "Error setting thread affinity on Linux: " << strerror(result)
              << std::endl;
  }
#endif
}