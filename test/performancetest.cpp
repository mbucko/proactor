

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <semaphore>
#include <thread>
#include <utility>

#include "proactor.h"

using ::testing::Eq;

class MathOperator {
 public:
  MathOperator(int64_t initial_value) : value_(initial_value) {}
  int64_t add(int64_t value) {
    value_ += value;
    return value;
  }
  int64_t subtract(int64_t value) {
    value_ -= value;
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
  static constexpr std::size_t kQueueSize = 100 * 1024 * 1024;

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
    for (int i = 0; i < kPartitions; ++i) {
      startLayer.process(i, &MathOperator::get,
                         [&semaphore](int64_t) { semaphore.release(); });
    }
    for (int i = 0; i < kPartitions; ++i) {
      semaphore.acquire();
    }
    for (int i = 0; i < kPartitions; ++i) {
      midLayer.process(i, &MathOperator::get,
                       [&semaphore](int64_t) { semaphore.release(); });
    }
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

TEST_F(PerformanceTest, Dummy) {
  constexpr uint64_t kMessages = 1 * 1000 * 1000ull;
  std::counting_semaphore<1> semaphore{0};
  for (uint64_t i = 0; i < kMessages; ++i) {
    const int key = static_cast<int>(i % kPartitions);
    addValue(key, 1);
  }
  wait();
  int64_t retrievedSum = 0ull;
  EXPECT_TRUE(endLayer.process(0, &MathOperator::get,
                               [&retrievedSum, &semaphore](int64_t sum) {
                                 retrievedSum = sum;
                                 semaphore.release();
                               }));
  semaphore.acquire();
  EXPECT_THAT(retrievedSum, kMessages);
}