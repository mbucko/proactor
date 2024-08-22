#ifndef PROACTORPARTITION_H
#define PROACTORPARTITION_H

#include <cstdint>
#include <functional>
#include <iostream>
#include <sstream>
#include <thread>
#include <utility>

#include "AdaptiveSleeper.h"
#include "ThreadAffinity.h"
#include "blockingconcurrentqueue.h"

template <typename COMPUTABLE>
class ProactorPartition {
 private:
  using Function = std::function<void(COMPUTABLE*)>;

 public:
  template <typename... Args>
  ProactorPartition(std::size_t capacity, std::size_t partition_index,
                    const Args&... args)
      : partition_index_(partition_index),
        computable_(args...),
        queue_(capacity),
        running_(true),
        thread_(&ProactorPartition::processQueue, this) {
    setThreadAffinity(thread_, partition_index_);
  }

  ~ProactorPartition() { stop(); }

  template <typename MemberFunc, typename Callback, typename... Args>
  bool process(MemberFunc func, Callback&& callback, Args&&... args) {
    static_assert(std::is_member_function_pointer_v<MemberFunc>,
                  "func must be a member function pointer");
    static_assert(std::is_invocable_v<MemberFunc, COMPUTABLE*, Args...>,
                  "Arguments provided to 'process()' function must match the "
                  "parameters of the COMPUTABLE member function");
    Function task = [func, callback = std::forward<Callback>(callback),
                     ... capturedArgs = std::forward<Args>(args)](
                        COMPUTABLE* computable) mutable {
      if constexpr (std::is_void_v<std::invoke_result_t<MemberFunc, COMPUTABLE*,
                                                        Args...>>) {
        std::invoke(func, computable, std::forward<Args>(capturedArgs)...);
        callback();
      } else {
        auto result =
            std::invoke(func, computable, std::forward<Args>(capturedArgs)...);
        callback(std::move(result));
      }
    };

    return queue_.enqueue(std::move(task));
  }

  void processQueue() {
    Function task;
    while (true) {
      while (queue_.try_dequeue(task)) [[likely]] {
        task(&computable_);
        sleeper_.reset();
      }

      [[unlikely]] if (!running_) { return; }

      sleeper_.sleep();
    }
  }

  void stop() noexcept {
    if (running_) {
      running_ = false;
      if (thread_.joinable()) {
        try {
          thread_.join();
        } catch (...) {
          std::ostringstream oss;
          oss << "Error: Failed to join worker thread (Partition: "
              << partition_index_ << ", Thread ID: " << thread_.get_id()
              << ").";
          std::cerr << oss.str() << std::endl;
        }
      }
    }
  }

 private:
  const std::size_t partition_index_;
  COMPUTABLE computable_;
  moodycamel::BlockingConcurrentQueue<Function> queue_;
  std::atomic<bool> running_;
  std::thread thread_;
  AdaptiveSleeper sleeper_;
};

#endif  // PROACTORPARTITION_H