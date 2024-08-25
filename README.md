# Proactor
The Proactor is a `partitioned`, `multi-threaded`, `asynchronous` task processing framework. It distributes tasks across multiple partitions based on a `key` and a `hash policy`, allowing for concurrent execution of tasks. The class statically allocates a number of partitions and uses a lock-free queue for efficient data passing into proactor's partitions.

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

    # Blocking
    # Process func on a partition associated to the key.
    process(key, func, callback, args...) : void

    # Process func on all partitions.
    process(func, callback, args...) : void

    # non-blocking
    # Process func on a partition associated to the key.
    try_process(key, func, callback, args...) : bool

    # Process func on all partitions.
    try_process(func, callback, args...) : bool

## Dependencies
The Proactor project relies on the following libraries and frameworks:
* Boost (version 1.51.0 or higher)
* Folly
* double-conversion
* gflags
* glog
* fmt
* OpenSSL
* Threads
* Google Test (for testing)

### Platform-specific dependencies:
* On Apple platforms: CoreFoundation framework
* On Unix platforms: pthread (Not yet tested)
* On Windows (Not yet implemented)