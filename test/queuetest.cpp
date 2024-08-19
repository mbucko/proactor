#include <gtest/gtest.h>

#include <thread>
#include <vector>

#include "lockfreequeue.h"

TEST(LockFreeQueueTest, ConstructorSetsCapacity) {
  LockFreeQueue<int> queue(10);
  EXPECT_EQ(10, queue.capacity());
}

TEST(LockFreeQueueTest, PushReturnsTrue) {
  LockFreeQueue<int> queue(5);
  EXPECT_TRUE(queue.push(1));
}

TEST(LockFreeQueueTest, PopReturnsFalseWhenEmpty) {
  LockFreeQueue<int> queue(5);
  int value;
  EXPECT_FALSE(queue.pop(&value));
}

TEST(LockFreeQueueTest, ConcurrentPop) {
  LockFreeQueue<int> queue(100);
  std::vector<std::thread> threads;
  std::atomic<int> successful_pops(0);

  for (int i = 0; i < 50; ++i) {
    queue.push(i);
  }

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back([&queue, &successful_pops]() {
      int value;
      if (queue.pop(&value)) {
        successful_pops++;
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  EXPECT_EQ(50, successful_pops);
}

TEST(LockFreeQueueTest, PushPopAndEmptyCheck) {
  LockFreeQueue<int> queue(5);
  std::vector<int> values = {1, 2, 3, 4, 5};

  for (int val : values) {
    EXPECT_TRUE(queue.push(val));
  }

  for (int expected : values) {
    int popped_value;
    EXPECT_TRUE(queue.pop(&popped_value));
    EXPECT_EQ(expected, popped_value);
  }

  EXPECT_TRUE(queue.empty());
  int value;
  EXPECT_FALSE(queue.pop(&value));
}