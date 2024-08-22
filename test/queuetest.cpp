#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <thread>
#include <vector>

#include "concurrentqueue.h"

using ::testing::Eq;

TEST(LockFreeQueueTest, PushReturnsTrue) {
  moodycamel::ConcurrentQueue<int> queue(4);
  EXPECT_TRUE(queue.enqueue(1));
}

TEST(LockFreeQueueTest, PopReturnsFalseWhenEmpty) {
  moodycamel::ConcurrentQueue<int> queue(4);
  int value;
  EXPECT_FALSE(queue.try_dequeue(value));
}

TEST(LockFreeQueueTest, ConcurrentPop) {
  moodycamel::ConcurrentQueue<int> queue(128);
  std::vector<std::thread> threads;
  std::atomic<int> successful_try_dequeues(0);

  for (int i = 0; i < 50; ++i) {
    queue.enqueue(i);
  }

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&queue, &successful_try_dequeues]() {
      int value;
      if (queue.try_dequeue(value)) {
        ++successful_try_dequeues;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }
  std::cout << "try_dequeues " << successful_try_dequeues << std::endl;
  EXPECT_EQ(50, successful_try_dequeues);
}

TEST(LockFreeQueueTest, PushPopAndEmptyCheck) {
  moodycamel::ConcurrentQueue<int> queue(4);
  std::vector<int> values = {0, 1, 2, 3};

  for (const int val : values) {
    EXPECT_TRUE(queue.enqueue(val));
  }

  for (int expected : values) {
    int try_dequeueped_value = -1;
    EXPECT_TRUE(queue.try_dequeue(try_dequeueped_value));
    ASSERT_EQ(expected, try_dequeueped_value);
  }

  EXPECT_THAT(queue.size_approx(), Eq(0));
  int value;
  EXPECT_FALSE(queue.try_dequeue(value));
}

// TODO add test with unique pointer