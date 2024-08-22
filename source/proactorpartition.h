#ifndef PROACTORPARTITION_H
#define PROACTORPARTITION_H

#include <cstdint>
#include <functional>
#include <sstream>
#include <thread>
#include <utility>

#include "concurrentqueue.h"
#include "threadaffinity.h"

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
      }

      if (!running_) {
        return;
      }

      // TODO add backout sleep policy
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
  moodycamel::ConcurrentQueue<Function> queue_;
  std::atomic<bool> running_;
  std::thread thread_;
};

#endif  // PROACTORPARTITION_H