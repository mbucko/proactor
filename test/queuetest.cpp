#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <vector>

#include "Queue.h"

using ::testing::Eq;

TEST(LockFreeQueueTest, PushReturnsTrue) {
  Queue<int> queue(4);
  EXPECT_TRUE(queue.enqueue(1));
}

TEST(LockFreeQueueTest, PopReturnsFalseWhenEmpty) {
  Queue<int> queue(4);
  int value;
  EXPECT_FALSE(queue.try_deque(value));
}

TEST(LockFreeQueueTest, ConcurrentPop) {
  Queue<int> queue(128);
  std::vector<std::thread> threads;
  std::atomic<int> successful_try_deques(0);

  for (int i = 0; i < 50; ++i) {
    queue.enqueue(i);
  }

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&queue, &successful_try_deques]() {
      int value;
      if (queue.try_deque(value)) {
        ++successful_try_deques;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
  EXPECT_EQ(50, successful_try_deques);
}

TEST(LockFreeQueueTest, PushPopAndEmptyCheck) {
  Queue<int> queue(4);
  std::vector<int> values = {0, 1, 2, 3};

  for (const int val : values) {
    EXPECT_TRUE(queue.enqueue(val));
  }

  for (int expected : values) {
    int try_dequeped_value = -1;
    EXPECT_TRUE(queue.try_deque(try_dequeped_value));
    ASSERT_EQ(expected, try_dequeped_value);
  }

  EXPECT_THAT(queue.size(), Eq(0));
  int value;
  EXPECT_FALSE(queue.try_deque(value));
}
