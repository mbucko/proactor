#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <semaphore>
#include <thread>

#include "Accumulator.h"
#include "Proactor.h"

using ::testing::Eq;
using namespace mbucko;
using namespace mbucko_test;

struct Hash {
  std::size_t operator()(int key) const { return key * 1009; }
};

class ProactorTest : public ::testing::Test {
 protected:
  using key_type = int;

  static constexpr std::size_t kPartitions = 10;
  static constexpr std::size_t kQueueSize = 1000;
  Proactor<key_type, Hash, kPartitions, Accumulator> proactor;
  ProactorTest() : proactor(kQueueSize, std::make_unique<uint32_t>(100u), 10) {}

  void TearDown() override { proactor.stop(); }
};

TEST_F(ProactorTest, BasicApi) {
  uint32_t retrievedSum0{0};
  uint32_t retrievedSum1{0};
  uint32_t retrievedSum2{0};
  std::counting_semaphore<kPartitions> semaphore{0};
  // Add 1 to pertition 0
  proactor.process(0, &Accumulator::add, []() {}, 1u);
  // Add 6 to pertition 1
  proactor.process(1, &Accumulator::add, []() {}, 6u);
  // Add 2 to pertition 0
  proactor.process(0, &Accumulator::add, []() {}, 2u);
  // Add 1 to all partitions
  proactor.process(&Accumulator::add, []() {}, 1u);
  proactor.process(0, &Accumulator::get,
                   [&retrievedSum0](uint32_t sum) { retrievedSum0 = sum; });
  proactor.process(1, &Accumulator::get,
                   [&retrievedSum1](uint32_t sum) { retrievedSum1 = sum; });
  proactor.process(2, &Accumulator::get,
                   [&retrievedSum2](uint32_t sum) { retrievedSum2 = sum; });
  // Signal all 10 partitions
  proactor.process(&Accumulator::get,
                   [&semaphore](uint32_t sum) { semaphore.release(); });

  for (int i = 0; i < kPartitions; ++i) {
    semaphore.acquire();
  }
  EXPECT_THAT(retrievedSum0, Eq(114u));
  EXPECT_THAT(retrievedSum1, Eq(117u));
  EXPECT_THAT(retrievedSum2, Eq(111u));
}