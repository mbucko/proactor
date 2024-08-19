#ifndef LOCKFREEQUEUE_H
#define LOCKFREEQUEUE_H

// TODO:
// Implement fixed-size lock-free SCSP queue
// https://www.youtube.com/watch?v=K3P_Lmq6pw0

#include <iostream>
#include <mutex>

template <typename T>
class LockFreeQueue {
 public:
  LockFreeQueue(std::size_t capacity)
      : capacity_(capacity),
        data_(new T[capacity]),
        head_(0),
        tail_(0),
        size_(0) {}

  ~LockFreeQueue() { delete[] data_; }

  bool push(const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == capacity_) {
      return false;
    }
    data_[tail_] = value;
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
    return true;
  }

  bool pop(T* value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == 0) {
      return false;
    }
    *value = data_[head_];
    head_ = (head_ + 1) % capacity_;
    --size_;
    return true;
  }

  std::size_t size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return size_;
  }

  std::size_t capacity() const { return capacity_; }

  std::size_t empty() const { return size() == 0; }

 private:
  const std::size_t capacity_;
  T* data_;
  std::size_t head_;
  std::size_t tail_;
  std::size_t size_;
  mutable std::mutex mutex_;
};

#endif  // LOCKFREEQUEUE_H