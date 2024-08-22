# Proactor
The Proactor is a `partitioned`, `multi-threaded`, `asynchronous` task processing framework. It distributes tasks across multiple partitions based on a `key` and a `hash policy`, allowing for concurrent execution of tasks. The class statically allocates a number of partitions and uses a lock-free queue for efficient data passing into the proactor's partitions.

## Features
* Fast task queueing via lock-free queues.
* Synchronous and asynchronous task enquing.
* Fixed number of partitions
* Thread Affinity

## Basic use
```C++
class Adder {
 public:
  Adder(uint32_t init_value) : value_(init_value) {}

  void add(uint32_t value) { value_ += value; }

  uint32_t get() const { return value_; }

 private:
  uint32_t value_;
};

static constexpr std::size_t kPartitions = 10;
static constexpr std::size_t kQueueSize = 1000;

struct HashPolicy {
  std::size_t operator()(int key) const { return key * 1009; }
};

Proactor<int, HashPolicy, kPartitions, Adder> proactor(kQueueSize, 0);

proactor.process(0, &Adder::add, []() {}, 1u);
proactor.process(2, &Adder::get, [&retrievedSum](uint32_t sum) {
  std::cout << "Sum: " << sum << std::endl;
});

proactor.stop();

```
## Full API (pseudocode):
    # Constructor
    Proactor(capacity, args...)

    # Sync
    # Process func on a partition associated to the key.
    process(key, func, callback, args...) : bool

    # Process func on aall partitions.
    process(func, callback, args...) : bool

    # Async (coming soon...)
    # Process func on a partition associated to the key.
    try_process(key, func, callback, args...) : bool

    # Process func on aall partitions.
    try_process(func, callback, args...) : bool

## Dependencies
* [googletest][googletest]
* [moodycamel::ConcurrentQueue][ConcurrentQueue]

[googletest]: https://github.com/google/googletest
[ConcurrentQueue]: https://github.com/cameron314/concurrentqueue