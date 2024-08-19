

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <semaphore>
#include <thread>

#include "accumulator.h"
#include "proactor.h"

using ::testing::Eq;

struct Hash {
  std::size_t operator()(int key) { return key * 1009; }
};

class ProactorTest : public ::testing::Test {
 protected:
  using key_type = int;
  static constexpr std::size_t nPartition = 10;
  Proactor<key_type, Hash, nPartition, Accumulator> proactor;

  ProactorTest() : proactor(10, 0) {}

  void TearDown() override { proactor.stop(); }
};

TEST_F(ProactorTest, Dummy) {
  uint32_t retrieved_sum{0};
  std::binary_semaphore signal{0};
  EXPECT_TRUE(proactor.process(0, &Accumulator::add, []() {}, 1u));
  EXPECT_TRUE(proactor.process(1, &Accumulator::add, []() {}, 6u));
  EXPECT_TRUE(proactor.process(0, &Accumulator::add, []() {}, 2u));
  EXPECT_TRUE(proactor.process(0, &Accumulator::get,
                               [&retrieved_sum, &signal](uint32_t sum) {
                                 retrieved_sum = sum;
                                 signal.release();
                               }));

  signal.acquire();
  EXPECT_THAT(retrieved_sum, Eq(3u));
}