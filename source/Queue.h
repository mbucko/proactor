#ifndef QUEUE_H
#define QUEUE_H

#include <mutex>

namespace mbucko {
template <typename T>
class Queue {
 public:
  Queue(std::size_t capacity)
      : capacity_(capacity),
        data_(new T[capacity]),
        head_(0),
        tail_(0),
        size_(0) {}

  ~Queue() { delete[] data_; }

  bool enqueue(const T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == capacity_) {
      return false;
    }
    data_[tail_] = value;
    tail_ = (tail_ + 1) % capacity_;
    ++size_;
    return true;
  }

  bool try_deque(T& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (size_ == 0) {
      return false;
    }
    value = data_[head_];
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

}  // namespace mbucko

#endif  // QUEUE_H