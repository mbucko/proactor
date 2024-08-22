#ifndef ADAPTIVESLEEPER_H
#define ADAPTIVESLEEPER_H

#include <chrono>
#include <thread>

namespace mbucko {

class AdaptiveSleeper {
 public:
  AdaptiveSleeper() : iteration_count_(0) {}

  void sleep() {
    [[likely]] if (iteration_count_ == 0) {
      std::this_thread::yield();
    } else {
      auto sleep_time = calculateSleepTime();
      std::this_thread::sleep_for(sleep_time);
    }
    ++iteration_count_;
  }

  void reset() { iteration_count_ = 0; }

 private:
  std::chrono::microseconds calculateSleepTime() const {
    [[likely]] if (iteration_count_ <= 10) {
      return std::chrono::microseconds(1);
    } else [[likely]] if (iteration_count_ <= 20) {
      return std::chrono::microseconds(10);
    } else [[likely]] if (iteration_count_ <= 30) {
      return std::chrono::microseconds(100);
    } else {
      // 1ms maximum
      return std::chrono::microseconds(1000);
    }
  }

 private:
  uint64_t iteration_count_;
};

}  // namespace mbucko

#endif  // ADAPTIVESLEEPER_H