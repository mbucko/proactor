

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <semaphore>
#include <thread>
#include <utility>

#include "Proactor.h"

using ::testing::Eq;
using namespace mbucko;

class MathOperator {
 public:
  MathOperator(int64_t initial_value) : value_(initial_value) {}
  int64_t add(int64_t value) {
    value_ += value;
    return value;
  }
  int64_t get() const { return value_; }

 private:
  int64_t value_;
};

struct Hash {
  std::size_t operator()(int key) const { return key; }
};

class PerformanceTest : public ::testing::Test {
 protected:
  using key_type = int;

  static constexpr std::size_t kPartitions = 10;
  static constexpr std::size_t kQueueSize = 128 * 1024;

  Proactor<key_type, Hash, 1, MathOperator> endLayer;
  Proactor<key_type, Hash, kPartitions, MathOperator> midLayer;
  Proactor<key_type, Hash, kPartitions, MathOperator> startLayer;

  PerformanceTest()
      : endLayer(kQueueSize, 0ull),
        midLayer(kQueueSize, 0ull),
        startLayer(kQueueSize, 0ull) {}

  void addValue(int key, int64_t value) {
    startLayer.process(
        key, &MathOperator::add,
        [key, this](int64_t value) {
          midLayer.process(
              key, &MathOperator::add,
              [key, this](int64_t value) {
                endLayer.process(
                    key, &MathOperator::add, [key](int64_t value) {}, value);
              },
              value);
        },
        value);
  }

  void wait() {
    std::counting_semaphore<kPartitions> semaphore{0};
    startLayer.process(&MathOperator::get,
                       [&semaphore](int64_t) { semaphore.release(); });
    for (int i = 0; i < kPartitions; ++i) {
      semaphore.acquire();
    }
    midLayer.process(&MathOperator::get,
                     [&semaphore](int64_t) { semaphore.release(); });
    for (int i = 0; i < kPartitions; ++i) {
      semaphore.acquire();
    }
    endLayer.process(0, &MathOperator::get,
                     [&semaphore](int64_t) { semaphore.release(); });
    semaphore.acquire();
  }

  ~PerformanceTest() {
    startLayer.stop();
    midLayer.stop();
    endLayer.stop();
  }
};

TEST_F(PerformanceTest, Timed_100M_Messages) {
  constexpr uint64_t kMessages = 10 * 1000 * 1000ull;
  std::counting_semaphore<1> semaphore{0};
  for (uint64_t i = 0; i < kMessages; ++i) {
    addValue(static_cast<int>(i % kPartitions), 1);
  }
  wait();
  std::atomic<int64_t> retrievedSum = 0ull;
  endLayer.process(0, &MathOperator::get,
                   [&retrievedSum, &semaphore](int64_t sum) {
                     retrievedSum = sum;
                     semaphore.release();
                   });
  semaphore.acquire();
  EXPECT_THAT(static_cast<int64_t>(retrievedSum), kMessages);
}